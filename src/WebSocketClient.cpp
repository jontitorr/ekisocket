#include <condition_variable>
#include <ekisocket/Errors.hpp>
#include <ekisocket/WebSocketClient.hpp>
#include <fmt/format.h>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

namespace {
constexpr std::chrono::seconds HEARTBEAT_INTERVAL { 30 };
constexpr std::chrono::minutes TIMEOUT_INTERVAL { 2 };
constexpr uint8_t MAX_HEADER_LENGTH { 14 };
/**
 * @brief Represents a WebSocket Data Frame. This is what is sent between the client and server. Some of the following
 * fields have been included for being correct, but left out as they are not needed/used.
 */
struct WSFrame {
    unsigned char fin : 1;
    // unsigned int rsv1 : 1; // Not used.
    // unsigned int rsv2 : 1; // Not used.
    // unsigned int rsv3 : 1; // Not used.
    unsigned char opcode : 4;
    unsigned char masked : 1;
    unsigned int payload_len : 7;
    uint64_t ext_payload_len;
    uint32_t masking_key;
    // uint64_t payload_data; // Not needed.
    unsigned char payload_start : 4; // Beginning of the payload, which can at most be 14.
};

void mask_payload(std::string& payload, uint32_t masking_key, uint8_t start = 0)
{
    for (size_t i = 0; i < payload.size() - start; ++i) {
        auto j = i % 4;
        // If j is 0, it represents the masking_key shifted by 24 bits (Octet 0).
        // If j is 1, it represents the masking_key shifted by 16 bits (Octet 1).
        // If j is 2, it represents the masking_key shifted by 8 bits (Octet 2).
        // If j is 3, it represents the masking_key shifted by 0 bits (Octet 3).
        *(reinterpret_cast<std::byte*>(&payload[start + i]))
            ^= std::byte { static_cast<uint8_t>((masking_key >> (24 - (j * 8))) & 0xFF) };
    }
}
} // namespace

namespace ekisocket::ws {
struct Client::Impl : http::Client {
    explicit Impl(std::string_view url)
        : m_url { url }
    {
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    ~Impl() override
    {
        set_automatic_reconnect(false);
        try {
            close();
        } catch (const errors::SSLClientError& e) {
            fmt::print(stderr, "Error closing WebSocket connection: {}\n", e.what());
        }
    }

    bool get_automatic_reconnect() const { return m_reconnect.load(); }

    const std::string& get_url() const
    {
        std::scoped_lock lk { m_mtx };
        return m_url;
    }

    void set_automatic_reconnect(bool reconnect) { m_reconnect = reconnect; }

    void set_on_message(const MessageCallback& cb)
    {
        std::scoped_lock lk { m_mtx };
        m_on_message = cb;
    }

    void set_url(std::string_view url)
    {
        std::scoped_lock lk { m_mtx };
        m_url = url;
    }

    bool send(std::string_view message) { return send_data(Opcode::TEXT, message); }

    void start()
    {
        do {
            if (!connect()) {
                return;
            }

            // When we are connected, we want to start sending heartbeats, so we can be notified when the connection is
            // closed.
            std::jthread heartbeat_thread(&Impl::heartbeat, this);
            // We also want to continuously read messages, so we can dispatch them to the m_on_message callback.
            std::jthread read_thread(&Impl::read, this);

            {
                std::scoped_lock lk { m_callback_mtx };
                if (m_on_message) {
                    m_on_message(Message { Opcode::OPEN, fmt::format("Connected to: {}", m_url) });
                }
            }

            // Continue to receive data until the connection is closed.
            while (m_status.load() != Status::CLOSED) {
                // Wait for the condition variable to be notified, and only unblock if there is work to be done.
                m_activity_flag.wait(false);
                poll();
            }
        } while (m_reconnect.load());
    }

    void start_async()
    {
        if (m_running_thread.joinable()) {
            return;
        }
        m_running_thread = std::jthread(&Impl::start, this);
    }

    void close(uint16_t code = 1000, std::string_view reason = "")
    {
        if (const auto status = m_status.load(); status == Status::CLOSING || status == Status::CLOSED) {
            return;
        }

        m_status = Status::CLOSING;

        // The message to send with this frame is the close code occupies 2 bytes, and the reason for closing can occupy
        // the rest.
        std::string data {};
        data.reserve(2 + reason.length());
        data.push_back(static_cast<char>(code >> 8));
        data.push_back(static_cast<char>(code));
        data += reason;
        send_data(Opcode::CLOSE, data);
    }

private:
    /**
     * @brief Connects to the endpoint.
     *
     * @return bool Whether or not the connection was successful.
     */
    bool connect()
    {
        if (const auto status = m_status.load(); status == Status::CONNECTING || status == Status::OPEN) {
            return false;
        }
        if (m_url.empty()) {
            throw errors::WebSocketClientError("URL not set.");
        }

        m_uri = http::URI::parse(m_url);

        if (m_uri.scheme != "ws" && m_uri.scheme != "wss") {
            return false;
        }

        // Set the scheme to its HTTP counterpart.
        m_uri.scheme = m_uri.scheme == "ws" ? "http" : "https";

        // Random 16 byte value that looks like it was encoded into base64.
        const auto key = util::get_random_base64_from(16);
        const http::Headers headers { { "Connection", "Upgrade" }, { "Upgrade", "websocket" },
            { "Sec-WebSocket-Version", "13" }, { "Sec-WebSocket-Key", key } };

        const auto res = http::Client::get(m_uri.to_string(), headers);

        if (res.status_code != 101) {
            return false;
        }

        const auto& r_headers = res.headers;

        if (!r_headers.contains("Upgrade") || !util::iequals(r_headers.at("Upgrade"), "websocket")) {
            return false;
        }
        if (!r_headers.contains("Connection") || !util::iequals(r_headers.at("Connection"), "Upgrade")) {
            return false;
        }
        if (!r_headers.contains("Sec-WebSocket-Accept")
            || r_headers.at("Sec-WebSocket-Accept") != util::compute_accept(key)) {
            return false;
        }

        m_status = Status::OPEN;
        return true;
    }

    /**
     * @brief Disconnects from the endpoint. This will disconnect from the server with no notification and should be
     * done when either the server is unreachable or the client is closing.
     */
    void disconnect(const Message& close_message)
    {
        if (const auto status = m_status.load(); !(status == Status::OPEN || status == Status::CLOSING)) {
            return;
        }

        m_status = Status::CLOSED;
        ssl().close();

        {
            std::scoped_lock lk { m_mtx };
            m_close_flags = { 0, 0 };

            m_read_buffer.clear();
            m_write_buffer = {};
        }

        {
            std::scoped_lock lk { m_callback_mtx };
            if (m_on_message) {
                m_on_message(close_message);
            }
        }

        m_activity_flag.test_and_set();
        m_activity_flag.notify_one();
        m_cv.notify_one();
        m_heartbeat_flag.clear();
        m_heartbeat_flag.notify_one();
        m_read_flag.clear();
        m_read_flag.notify_one();
    }

    /**
     * @brief Sends payload data to the server parsed as a WebSocket frame. The opcode must be provided so as to express
     * the type of data being sent.
     *
     * @param opcode The opcode of the WebSocket frame.
     * @param message The payload data to send.
     * @return bool Whether or not the data was sent successfully.
     */
    bool send_data(const Opcode& opcode, std::string_view data)
    {
        if (const auto status = m_status.load(); !(status == Status::OPEN || status == Status::CLOSING)) {
            return false;
        }

        //* Masking key should be a random 32-bit integer.
        const auto masking_key = util::get_random_number();

        // We can use a uint8_t vector to hold the frame.
        std::string frame {};
        frame.reserve(data.length() + MAX_HEADER_LENGTH);

        //* First byte is the FIN bit, the opcode, and the mask bit.
        frame.push_back(static_cast<char>(0x80 | static_cast<uint8_t>(opcode)));

        //* Second byte is the mask bit and the length of the payload.
        //* If the payload length is less than 126, then we can send it in one byte.
        if (const uint64_t payload_length = data.length(); payload_length < 126) {
            // Push back the payload_length with the mask bit.
            frame.push_back(static_cast<char>(payload_length | 0x80));
        }
        //* If the payload length is greater than 126, then we need to send it in 2 extra bytes.
        else if (payload_length < 65536) {
            // Push back 126 with the mask bit.
            frame.push_back(static_cast<char>(126 | 0x80));
            frame.push_back(static_cast<char>(payload_length >> 8));
            frame.push_back(static_cast<char>(payload_length));
        }
        //* If the payload length is greater than 65536, then we need to send it in 8 extra bytes.
        else {
            // Push back 127 with the mask bit.
            frame.push_back(static_cast<char>(127 | 0x80));
            frame.push_back(static_cast<char>(payload_length >> 56));
            frame.push_back(static_cast<char>(payload_length >> 48));
            frame.push_back(static_cast<char>(payload_length >> 40));
            frame.push_back(static_cast<char>(payload_length >> 32));
            frame.push_back(static_cast<char>(payload_length >> 24));
            frame.push_back(static_cast<char>(payload_length >> 16));
            frame.push_back(static_cast<char>(payload_length >> 8));
            frame.push_back(static_cast<char>(payload_length));
        }

        // Add the masking key to the frame.
        frame.push_back(static_cast<char>(masking_key >> 24));
        frame.push_back(static_cast<char>((masking_key >> 16) & 0xFF));
        frame.push_back(static_cast<char>((masking_key >> 8) & 0xFF));
        frame.push_back(static_cast<char>(masking_key & 0xFF));

        const auto header_length = static_cast<uint8_t>(frame.size());

        frame.insert(frame.end(), data.begin(), data.end());

        // We want to only mask the payload data.
        mask_payload(frame, masking_key, header_length);

        {
            std::scoped_lock lk { m_mtx };
            m_write_buffer.emplace(std::move(frame));
        }

        m_activity_flag.test_and_set();
        m_activity_flag.notify_one();
        return true;
    }

    /**
     * @brief Parses incoming frame data from the server as a WebSocket frame.
     *
     * @param message The message to parse.
     */
    void process_data(std::string& data)
    {
        // Because of the edge case of the first frame being so big it takes most of the buffer, having an incomplete
        // (less than 2 bytes) frame remaining, we have to check if there's a leftover byte.
        if (m_leftover_byte) {
            data.insert(0, 1, static_cast<char>(m_leftover_byte.value()));
            m_leftover_byte = std::nullopt;
        }

        // If we've reached our base case or the message is genuinely too short to be considered a frame, then return.
        if (data.empty() || data.length() < 2) {
            return;
        }

        // Processing the data is just the reverse of sending it.
        WSFrame f {};
        f.fin = (static_cast<std::byte>(data[0]) & std::byte { 0x80 }) == std::byte { 0x80 };
// Get the opcode.
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
        f.opcode = std::to_integer<uint8_t>(static_cast<std::byte>(data[0]) & std::byte { 0x0F });
        f.masked = (static_cast<std::byte>(data[1]) & std::byte { 0x80 }) == std::byte { 0x80 };
        f.payload_start = 2;
        f.payload_len = std::to_integer<unsigned int>(static_cast<std::byte>(data[1]) & std::byte { 0x7F });
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif
        if (f.payload_len == 126) {
            f.payload_start += 2;
        } else if (f.payload_len == 127) {
            f.payload_start += 8;
        }
        // If we have a mask bit we must add 4 bytes to the expected header length.
        if (f.masked) {
            f.payload_start += 4;
        }

        // If for some reason we do not have the complete header for the frame, we must fetch it.
        if (data.length() < f.payload_start) {
            auto needed = f.payload_start - data.length();
            do {
                const auto next_message = ssl().receive(needed);

                if (next_message.empty()) {
                    continue;
                }

                data += next_message;
                needed -= next_message.length();
            } while (needed > 0);
        }

        // If the payload length is 126, then we can get the payload length from the next 2 bytes.
        if (f.payload_len == 126) {
            f.ext_payload_len = static_cast<uint64_t>(static_cast<uint8_t>(data[2])) << 8U;
            f.ext_payload_len |= static_cast<uint64_t>(static_cast<uint8_t>(data[3]));
        }
        // If the payload length is 127, then we need to read the next 8 bytes.
        else if (f.payload_len == 127) {
            // Use static_cast to prevent overflows.
            f.ext_payload_len = static_cast<uint64_t>(static_cast<uint8_t>(data[2])) << 56U;
            f.ext_payload_len |= static_cast<uint64_t>(static_cast<uint8_t>(data[3])) << 48U;
            f.ext_payload_len |= static_cast<uint64_t>(static_cast<uint8_t>(data[4])) << 40U;
            f.ext_payload_len |= static_cast<uint64_t>(static_cast<uint8_t>(data[5])) << 32U;
            f.ext_payload_len |= static_cast<uint64_t>(static_cast<uint8_t>(data[6])) << 24U;
            f.ext_payload_len |= static_cast<uint64_t>(static_cast<uint8_t>(data[7])) << 16U;
            f.ext_payload_len |= static_cast<uint64_t>(static_cast<uint8_t>(data[8])) << 8U;
            f.ext_payload_len |= static_cast<uint64_t>(static_cast<uint8_t>(data[9]));
        }
        // Get the masking key, if the mask bit is set.
        if (f.masked) {
            f.masking_key = static_cast<uint32_t>(static_cast<uint8_t>(data[f.payload_start])) << 24U;
            f.masking_key |= static_cast<uint32_t>(static_cast<uint8_t>(data[f.payload_start + 1])) << 16U;
            f.masking_key |= static_cast<uint32_t>(static_cast<uint8_t>(data[f.payload_start + 2])) << 8U;
            f.masking_key |= static_cast<uint32_t>(static_cast<uint8_t>(data[f.payload_start + 3]));
        }

        // Corresponds to the length of the frame received, which can contain more than just the payload. For example,
        // parts of another frame.
        const size_t actual_payload_len = data.length() - f.payload_start;

        // Our payload length can either be the extended or the normal payload length.
        const auto expected_payload_len
            = static_cast<size_t>(f.ext_payload_len > 0 ? f.ext_payload_len : f.payload_len);

        if (actual_payload_len < expected_payload_len) {
            auto needed = expected_payload_len - actual_payload_len;
            do {
                const auto next_message = ssl().receive(needed);

                if (next_message.empty()) {
                    continue;
                }

                data += next_message;
                needed -= next_message.length();
            } while (needed > 0);
        }

        std::string payload_data { data.substr(f.payload_start, expected_payload_len) };

        // Create a Message to dispatch to the m_on_message callback.
        Message dispatch {};
        // We do not want to send our heartbeats.
        bool should_dispatch { true };

        // If the opcode is a message, then we need to unmask the payload.
        switch (Opcode { f.opcode }) {
        case Opcode::BINARY:
        case Opcode::CONTINUATION:
        case Opcode::TEXT: {
            if (f.masked) {
                mask_payload(payload_data, f.masking_key);
            }

            {
                std::unique_lock lk { m_mtx };
                m_read_buffer += payload_data;
                if (f.fin) {
                    // We have to return the current read buffer.
                    dispatch.data = std::move(m_read_buffer);
                    m_read_buffer.clear();
                }
            }
            break;
        }
        case Opcode::PING: {
            // If the opcode is PING, then we need to send a PONG.
            // A PONG consists of us sending the data we received back ensure that we are responsive.
            if (f.masked) {
                // We need to unmask the payload.
                mask_payload(payload_data, f.masking_key);
            }

            // Echo the payload back to the client.
            send_data(Opcode::PONG, payload_data);
            dispatch.data = std::move(payload_data);
            break;
        }
        case Opcode::PONG: {
            // If we received our heartbeat message, we can reset the number of missed heartbeats.
            if (payload_data == m_heartbeat_message) {
                m_missed_heartbeats = 0;
                should_dispatch = false;
            }
            break;
        }
        case Opcode::CLOSE: {
            should_dispatch = false;
            // If the opcode is CLOSE, then we need to close the connection.
            m_close_flags.server = 1;

            // If payload data is not empty, then we received information regarding why we are closing.
            if (!payload_data.empty()) {
                m_close_message.emplace();
                //* MUST contain a 2-byte unsigned integer (in network byte order) representing a status code indicating
                // the reason for closure.
                m_close_message->code
                    = static_cast<uint16_t>(static_cast<uint16_t>(static_cast<uint8_t>(payload_data[0])) << 8U);
                m_close_message->code |= static_cast<uint16_t>(static_cast<uint8_t>(payload_data[1]));
                //* Anything past the two bytes is to be considered a UTF-8 encoded string denoting the reason for
                // closing.
                m_close_message->data = payload_data.substr(2);
            }

            close();
            break;
        }
        default: {
            close();
            dispatch.type = Opcode::BAD;
            dispatch.data = fmt::format("Received unknown opcode: {}", std::to_string(f.opcode));
            break;
        }
        }

        dispatch.type = static_cast<Opcode>(f.opcode);

        // Erase the message until we have processed the entire frame.
        data.erase(0, f.payload_start + expected_payload_len);

        // If there is 1 byte leftover in the message, store it.
        if (data.length() == 1) {
            m_leftover_byte = data[0];
        }

        {
            std::scoped_lock lk { m_callback_mtx };
            if (m_on_message && should_dispatch) {
                m_on_message(dispatch);
            }
        }

        if (!data.empty()) {
            return process_data(data);
        }
    }

    /**
     * @brief Polls the WebSocket for new data, using the timeout specified on construction.
     */
    void poll()
    {
        if (m_status.load() == Status::CLOSED) {
            return;
        }
        if (auto data = ssl().receive(); !data.empty()) {
            process_data(data);
        }

        {
            // Check if we should close the connection.
            std::unique_lock lk { m_mtx };
            std::string reason {};

            if ((m_close_flags.client && m_close_flags.server && !reason.append("Mutual disconnection.").empty())
                || (m_close_flags.client && std::chrono::steady_clock::now() > m_close_timeout
                    && !reason.append("Connection closed because server took too long to send close frame.").empty())
                || (!ssl().connected() && !reason.append("No longer connected to the socket.").empty())) {
                lk.unlock();
                return disconnect(m_close_message.value_or(Message { .type = Opcode::CLOSE, .data = reason }));
            }
        }

        {
            std::scoped_lock lk { m_mtx };
            while (!m_write_buffer.empty()) {
                // Because we can have data queued ahead of our CLOSE frame in the buffer, we need to be able to cancel
                // sending that data, since when we send the CLOSE frame, we will not be able to send any more data. We
                // can peek at the first bytes of each of our messages in the write buffer, and look for the CLOSE
                // frame.

                const auto message = std::move(m_write_buffer.front());
                m_write_buffer.pop();

                auto sent = ssl().send(message);

                while (sent < message.length()) {
                    sent += ssl().send(std::string_view { message }.substr(sent));
                }
                // If that message was a close frame, empty the rest of the write buffer.
                if ((static_cast<std::byte>(message[0]) & std::byte { 0xF }) == static_cast<std::byte>(Opcode::CLOSE)) {
                    m_close_flags.client = 1;
                    // Start counting the timer, although we haven't sent the CLOSE frame yet.
                    m_close_timeout = std::chrono::steady_clock::now() + TIMEOUT_INTERVAL;
                    m_write_buffer = {};
                }
                // If the message was our heartbeat, notify the thread.
                else if ((static_cast<std::byte>(message[0]) & std::byte { 0xF })
                    == static_cast<std::byte>(Opcode::PING)) {
                    m_heartbeat_flag.clear();
                    m_heartbeat_flag.notify_one();
                }
            }
        }

        // We have completed all work to be done.
        m_activity_flag.clear();
        m_activity_flag.notify_one();
        m_read_flag.clear();
        m_read_flag.notify_one();
    }

    /**
     * @brief Continuously sends heartbeats to the server (PING frames), to ensure the connection is still alive.
     */
    void heartbeat()
    {
        while (m_status.load() == Status::OPEN && send_data(Opcode::PING, m_heartbeat_message)) {
            ++m_missed_heartbeats;
            m_heartbeat_flag.test_and_set();
            m_heartbeat_flag.wait(true);

            if (m_status.load() != Status::OPEN) {
                break;
            }

            {
                std::unique_lock lk { m_heartbeat_mtx };
                m_cv.wait_for(lk, HEARTBEAT_INTERVAL, [this] { return m_status.load() != Status::OPEN; });
            }

            if (m_missed_heartbeats >= 3) {
                return disconnect(Message { .type = Opcode::CLOSE, .data = "Too many missed heartbeats." });
            }
        }
    }

    /**
     * @brief Waits until there is data to be read from the WebSocket. This is a blocking operation used for polling.
     */
    void read()
    {
        while ((m_status.load() != Status::CLOSED && ssl().query(true, false)) || m_status.load() == Status::CLOSING) {
            m_read_flag.test_and_set(); // We found data to be read.
            m_activity_flag.test_and_set(); // Notify the main thread that we have work to do.
            m_activity_flag.notify_one();
            m_read_flag.wait(true); // Wait for the main thread to finish processing the data.
        }
    }

    /// The current status of the WebSocket connection.
    std::atomic<Status> m_status {};
    /// The URI of the WebSocket connection.
    http::URI m_uri {};
    /// The callback function to be called when a message is received.
    MessageCallback m_on_message {};
    /// Buffer containing the data read from the WebSocket.
    std::string m_read_buffer {};
    /// Leftover byte from the previous read operation.
    std::optional<uint8_t> m_leftover_byte {};
    /// Buffer containing data to be sent to the server.
    std::queue<std::string> m_write_buffer {};
    /// The URL the client is currently connected/connecting to.
    std::string m_url {};
    /// Mutex for thread safety.
    mutable std::mutex m_mtx {};
    /// Mutex for callbacks, needs to be recursive.
    mutable std::recursive_mutex m_callback_mtx {};
    /// Mutex for the heartbeat sleep.
    mutable std::mutex m_heartbeat_mtx {};
    /// Stored message for relaying the close information when connection is closed.
    std::optional<Message> m_close_message {};
    /// Flags for indicating which side has sent over the CLOSE frame.
    struct CloseFlags {
        uint8_t client : 1;
        uint8_t server : 1;
    };

    CloseFlags m_close_flags { 0, 0 };
    /// Close timeout, in case the server does not response with a close frame in time.
    std::chrono::steady_clock::time_point m_close_timeout {};
    /// Atomic Flag for indicating whether there is network activity or not.
    std::atomic_flag m_activity_flag = ATOMIC_FLAG_INIT;
    // Atomic Flag for the read thread to be notified when to start reading again.
    std::atomic_flag m_read_flag = ATOMIC_FLAG_INIT;
    /// Whether or not to reconnect to the server if the connection is lost (Defaults to true).
    std::atomic_bool m_reconnect { true };
    /// String representing the heartbeat message to send.
    std::string m_heartbeat_message { "--heartbeat--" };
    /// Atomic flag for the heartbeat thread to be notified when to start sending heartbeats again.
    std::atomic_flag m_heartbeat_flag = ATOMIC_FLAG_INIT;
    /// Condition variable for sleeping the heartbeat thread.
    std::condition_variable m_cv {};
    /// Number of missed heartbeats.
    std::atomic_uint8_t m_missed_heartbeats {};
    /// Thread used for running the WebSocket client on a seperate thread, not blocking the calling thread.
    std::jthread m_running_thread {};
};

Client::Client(std::string_view url)
    : m_impl { std::make_unique<Client::Impl>(url) }
{
}
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;
Client::~Client() = default;

bool Client::get_automatic_reconnect() const { return m_impl->get_automatic_reconnect(); }

const std::string& Client::get_url() const { return m_impl->get_url(); }

void Client::set_automatic_reconnect(bool reconnect) const { return m_impl->set_automatic_reconnect(reconnect); }

void Client::set_on_message(const MessageCallback& cb) const { return m_impl->set_on_message(cb); }

void Client::set_url(std::string_view url) const { return m_impl->set_url(url); }

bool Client::send(std::string_view message) const { return m_impl->send(message); }

void Client::start() const { return m_impl->start(); }

void Client::start_async() const { return m_impl->start_async(); }

void Client::close(uint16_t code, std::string_view reason) const { return m_impl->close(code, reason); }

} // namespace ekisocket::ws

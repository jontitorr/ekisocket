#include <algorithm>
#include <ekisocket/HttpClient.hpp>
#include <fmt/format.h>
#include <numeric>
#include <unordered_map>

namespace {
constexpr uint16_t HTTP_PORT { 80 };
constexpr uint16_t HTTPS_PORT { 443 };

const std::unordered_map<ekisocket::http::Method, std::string> METHODS {
    { ekisocket::http::Method::GET, "GET" },
    { ekisocket::http::Method::POST, "POST" },
    { ekisocket::http::Method::PUT, "PUT" },
    { ekisocket::http::Method::DELETE_, "DELETE" },
    { ekisocket::http::Method::HEAD, "HEAD" },
    { ekisocket::http::Method::OPTIONS, "OPTIONS" },
    { ekisocket::http::Method::CONNECT, "CONNECT" },
    { ekisocket::http::Method::TRACE, "TRACE" },
    { ekisocket::http::Method::PATCH, "PATCH" },
};
} // namespace

namespace ekisocket::http {
struct Client::Impl : ssl::Client {
    explicit Impl()
        : ssl::Client({}, {})
    {
    }

    [[nodiscard]] Response request(const Method& method, std::string_view url, const Headers& headers,
        std::string_view body, bool keep_alive = false, bool stream = false, const BodyCallback& cb = {})
    {
        auto uri = Uri::parse(url);

        if (uri.scheme.empty()) {
            uri.scheme = "http";
        }
        if (!util::iequals(uri.scheme, "http") && !util::iequals(uri.scheme, "https")) {
            throw errors::HttpClientError(fmt::format("Invalid scheme: {}", uri.scheme));
        }
        if (!uri.port.has_value()) {
            uri.port = util::iequals(uri.scheme, "http") ? HTTP_PORT : HTTPS_PORT;
        }
        if (ssl::Client::connected()) {
            // We need to select our ssl_client, if available, to trigger our disconnect discovery.
            // Should already be non-blocking so as to not block forever.
            ssl::Client::set_blocking(false);
            // Perform a quick read.
            (void)ssl::Client::receive(0);
            // Go back to blocking.
            ssl::Client::set_blocking(true);
            // We should have detected whether we were disconnected or not.
        }
        if (auto requested_server = fmt::format("{}:{}", uri.host, uri.port.value());
            m_connected_to.empty() || m_connected_to != requested_server || !ssl::Client::connected()) {
            ssl::Client::set_hostname(uri.host);
            ssl::Client::set_port(uri.port.value());
            ssl::Client::set_use_ssl(uri.port == HTTPS_PORT);
            ssl::Client::close();
            if (!ssl::Client::connect()) {
                throw errors::HttpClientError(fmt::format("Failed to connect to {}:{}", uri.host, uri.port.value()));
            }
            m_connected_to = std::move(requested_server);
        }
        if (!METHODS.contains(method)) {
            throw errors::HttpClientError(fmt::format("Invalid method: {}", static_cast<uint8_t>(method)));
        }
        if (uri.path.empty()) {
            uri.path = "/";
        }
        if (!uri.query.empty()) {
            // Convert the query to a string.
            uri.path += '?'
                + std::accumulate(std::next(uri.query.begin()), uri.query.end(),
                    fmt::format("{}={}", uri.query.begin()->first, uri.query.begin()->second),
                    [](std::string a, const auto& pair) { return a + fmt::format("&{}={}", pair.first, pair.second); });
        }
        if (!uri.fragment.empty()) {
            uri.path += fmt::format("#{}", uri.fragment);
        }

        auto line = fmt::format("{} {} {}\r\n", METHODS.at(method), uri.path, "HTTP/1.1");

        line += uri.port == HTTPS_PORT || uri.port == HTTP_PORT
            ? fmt::format("Host: {}\r\n", uri.host)
            : fmt::format("Host: {}:{}\r\n", uri.host, uri.port.value());

        for (const auto& [key, value] : headers) {
            line += fmt::format("{}: {}\r\n", key, value);
        }

        if (!keep_alive) {
            line += "Connection: close\r\n";
            m_connected_to.clear();
        }
        if (!body.empty()) {
            line += fmt::format("Content-Length: {}\r\n", body.length());
        }

        line += "\r\n"; // End of headers.
        line += body;

        m_streaming = stream;
        m_body_callback = cb;

        auto sent = ssl::Client::send(line);

        while (sent < line.length()) {
            sent += ssl::Client::send(std::string_view { line }.substr(sent));
        }

        return receive();
    }

    [[nodiscard]] ssl::Client& ssl() { return static_cast<ssl::Client&>(*this); }

private:
    static void parse_chunked(std::string& body)
    {
        std::string new_body {};

        for (size_t i {}; i < body.length(); i += 2) {
            // If we have reached the end of the chunked body, we are done.
            if (body[i] == '0' && body[i + 1] == '\r' && body[i + 2] == '\n' && body[i + 3] == '\r'
                && body[i + 4] == '\n') {
                break;
            }

            auto chunk_size_start = i;

            // Skip until we are the end of the chunk size.
            while (body[i] != '\r' && body[i + 1] != '\n') {
                ++i;
            }

            const auto chunk_size_end = i;

            // Skip the CRLF.
            i += 2;

            const auto chunk_size_str = body.substr(chunk_size_start, chunk_size_end - chunk_size_start);
            // We now have the chunk size (which is always a hex number).
            const auto chunk_size = std::stoul(chunk_size_str, nullptr, 16);

            new_body.reserve(new_body.length() + chunk_size);
            new_body.append(body.substr(i, chunk_size));
            i += chunk_size;
        }

        body = std::move(new_body);
    }

    /**
     * @brief Receives data from the server.
     *
     * @return Response The response from the server.
     */
    Response receive()
    {
        Response res {};
        std::string response {};
        size_t end_of_headers {};

        do {
            const auto old_length = response.length();
            response += ssl::Client::receive();
            end_of_headers = response.find("\r\n\r\n", old_length);
        } while (end_of_headers == std::string::npos);

        // We know the body would be anything after the end of the headers.
        auto body = response.substr(end_of_headers + 4);

        // We no longer need the body contained within the headers.
        response.resize(end_of_headers + 2);

        // The first occurrence of a CRLF would mark the end of the status line.
        auto end_of_status_line = response.find("\r\n");

        if (end_of_status_line == std::string::npos) {
            throw errors::HttpClientError(fmt::format("Invalid status line: {}", response));
        }

        // Split the status line into status code and status message.
        auto status_line = util::split(std::string_view { response }.substr(0, end_of_status_line), " ");

        if (!util::is_number(status_line[1])) {
            throw errors::HttpClientError(fmt::format("Invalid status code: {}", status_line[1]));
        }

        res.status_code = static_cast<uint16_t>(std::stoul(status_line[1]));
        res.status_message = util::join(status_line.begin() + 2, status_line.end(), " ");

        // Set the headers.
        size_t content_length {};
        std::string key {};
        std::string value {};
        auto j = end_of_status_line + 2; // Used for marking the beginning of our value.
        bool encoded {};

        for (auto i = j; i < response.length(); ++i) {
            if (response[i] != ':') {
                continue;
            }

            // If we have a colon, we know we are at the key.
            // We know we are at the key, so we can set the key to the string from the beginning of the response to the
            // colon.
            key = response.substr(j, i - j);

            // We need to skip the colon and any spaces after it.
            while (response[i] == ':' || response[i] == ' ') {
                ++i;
            }

            // We are now at the value.
            j = i;

            // If we have a CRLF, we know we are at the end of the value.
            while (response[i] != '\r' && response[i] != '\n') {
                ++i;
            }

            // We know we are at the end of the value, so we can set the value to the string from the beginning of the
            // response to the CRLF.
            value = response.substr(j, i - j);

            // If we received a body.
            if (util::iequals(key, "Content-Length")) {
                content_length = std::stoul(value);
            }

            // We can now set the headers.
            res.headers.try_emplace(std::move(key), std::move(value));
            j = i + 2;
        }

        // We really only care about chunked encoding.
        if (res.headers.contains("Transfer-Encoding") && res.headers["Transfer-Encoding"] == "chunked") {
            encoded = true;
        }

        auto bytes_received { body.length() };

        // If body is not empty, we need to stream the current body to the callback.
        if (!body.empty() && m_streaming && m_body_callback) {
            m_body_callback(body);
        }

        while (bytes_received < content_length) {
            const auto chunk = ssl::Client::receive((std::max)(content_length - bytes_received, 4096ULL));

            if (chunk.empty()) {
                continue;
            }
            if (m_streaming && m_body_callback) {
                // If we are streaming, call the callback.
                m_body_callback(chunk);
            } else {
                // Otherwise, append the chunk to the body.
                body += chunk;
            }

            bytes_received += chunk.length();
        }

        if (encoded) {
            auto end_of_chunk { body.find("0\r\n\r\n") };

            // If there is no end of chunk marker, we must receive until we have it.
            while (end_of_chunk == std::string::npos) {
                const auto old_length = body.length();

                body += ssl::Client::receive();
                // We can safely skip the old length, since we know there is no end of chunk marker there.
                end_of_chunk = body.find("0\r\n\r\n", old_length);
            }

            parse_chunked(body);
        }

        res.body = std::move(body);
        return res;
    }

    /// Used for keeping track of the currently connected to server.
    std::string m_connected_to {};
    /// Used for determining whether or not to stream the response.
    bool m_streaming {};
    /// Used for keeping track of the callback to call for each chunk of data received.
    BodyCallback m_body_callback {};
};

#define DEFINE_HTTP_FUNCTION(name, method)                                                                             \
    Response Client::name(                                                                                             \
        std::string_view url, const Headers& headers, std::string_view body, bool stream, const BodyCallback& cb)      \
    {                                                                                                                  \
        return request(Method::method, url, headers, body, true, stream, cb);                                          \
    }

#define DEFINE_OUTSIDE_FUNCTION(name, method)                                                                          \
    Response name(                                                                                                     \
        std::string_view url, const Headers& headers, std::string_view body, bool stream, const BodyCallback& cb)      \
    {                                                                                                                  \
        return request(Method::method, url, headers, body, stream, cb);                                                \
    }
DEFINE_OUTSIDE_FUNCTION(get, GET)
DEFINE_OUTSIDE_FUNCTION(post, POST)
DEFINE_OUTSIDE_FUNCTION(put, PUT)
DEFINE_OUTSIDE_FUNCTION(delete_, DELETE_)
DEFINE_OUTSIDE_FUNCTION(head, HEAD)
DEFINE_OUTSIDE_FUNCTION(options, OPTIONS)
DEFINE_OUTSIDE_FUNCTION(connect, CONNECT)
DEFINE_OUTSIDE_FUNCTION(trace, TRACE)
DEFINE_OUTSIDE_FUNCTION(patch, PATCH)

Response request(const Method& method, std::string_view url, const Headers& headers, std::string_view body, bool stream,
    const BodyCallback& cb)
{
    return Client().request(method, url, headers, body, false, stream, cb);
}

Client::Client()
    : m_impl { std::make_unique<Client::Impl>() }
{
}

Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;
Client::~Client() = default;

DEFINE_HTTP_FUNCTION(get, GET)
DEFINE_HTTP_FUNCTION(post, POST)
DEFINE_HTTP_FUNCTION(put, PUT)
DEFINE_HTTP_FUNCTION(delete_, DELETE_)
DEFINE_HTTP_FUNCTION(head, HEAD)
DEFINE_HTTP_FUNCTION(options, OPTIONS)
DEFINE_HTTP_FUNCTION(connect, CONNECT)
DEFINE_HTTP_FUNCTION(trace, TRACE)
DEFINE_HTTP_FUNCTION(patch, PATCH)

Response Client::request(const Method& method, std::string_view url, const Headers& headers, std::string_view body,
    bool keep_alive, bool stream, const BodyCallback& cb) const
{
    return m_impl->request(method, url, headers, body, keep_alive, stream, cb);
}

ssl::Client& Client::ssl() const { return m_impl->ssl(); }
} // namespace ekisocket::http

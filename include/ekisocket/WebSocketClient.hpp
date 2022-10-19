#pragma once
#include <ekisocket/HttpClient.hpp>

namespace ekisocket::ws {
/**
 * @brief Represents the status of a WebSocket connection.
 */
enum class Status { CLOSED, CLOSING, CONNECTING, OPEN };

/**
 * @brief Represents the type of a WebSocket message.
 */
enum class Opcode : uint8_t {
    CONTINUATION = 0x0,
    TEXT = 0x1,
    BINARY = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA,
    BAD = 20, // Defined for returning errors.
    OPEN = 30, // Defined for signaling opening of a WebSocket connection.
};

/**
 * @brief Represents a WebSocket message.
 */
struct Message {
    /// The type of message received.
    Opcode type {};
    /// The message data, will be the close reason if the type is CLOSE.
    std::string data {};
    /// The close code, if the message is a close message.
    uint16_t code {};
};

using MessageCallback = std::function<void(const Message& message)>;

/**
 * @brief Represents a simple WebSocket client, can perform WebSocket connections with no support for extensions.
 */
class Client {
public:
    EKISOCKET_EXPORT explicit Client(std::string_view url = "");
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    EKISOCKET_EXPORT Client(Client&&) noexcept;
    EKISOCKET_EXPORT Client& operator=(Client&&) noexcept;
    EKISOCKET_EXPORT ~Client();

    [[nodiscard]] EKISOCKET_EXPORT bool get_automatic_reconnect() const;

    /**
     * @brief Get the url that the client is currently connected to.
     *
     * @return const std::string& The url that the client is currently connected to.
     */
    [[nodiscard]] EKISOCKET_EXPORT const std::string& get_url() const;

    /**
     * @brief Sets whether or not automatic reconnection should be enabled or not. If on, when disconnecting, the client
     * will attempt to reconnect.
     *
     * @param reconnect_ Whether or not to reconnect automatically when disconnected.
     */
    EKISOCKET_EXPORT void set_automatic_reconnect(bool reconnect) const;

    /**
     * @brief Set a callback function to be called when a message is received.
     *
     * @param cb The callback function.
     */
    EKISOCKET_EXPORT void set_on_message(const MessageCallback& cb) const;

    /**
     * @brief Set the url to connect to. If there are any query parameters, they will be parsed and added as well.
     *
     * @param url The url to connect to.
     */
    EKISOCKET_EXPORT void set_url(std::string_view url) const;

    /**
     * @brief Sends payload data of type Text to the server.
     *
     * @param message The payload data to send.
     * @return bool Whether or not the message was sent successfully.
     */
    EKISOCKET_EXPORT bool send(std::string_view message) const;

    /**
     * @brief Starts the WebSocket and connects to the server, polling for messages. This will be blocking.
     */
    EKISOCKET_EXPORT void start() const;

    /**
     * @brief Starts the WebSocket connection, but on a seperate thread. Good for long-running connections.
     */
    EKISOCKET_EXPORT void start_async() const;

    /**
     * @brief Closes the WebSocket connection, sending a close frame with a optional message stating the reason for the
     * closure.
     *
     * @param reason The reason for the closure.
     */
    EKISOCKET_EXPORT void close(uint16_t code = 1000, std::string_view reason = "") const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl {};
};
} // namespace ekisocket::ws

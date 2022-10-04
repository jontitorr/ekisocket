#pragma once
#include <ekisocket/Errors.hpp>
#include <ekisocket_export.h>
#include <memory>
#include <string>

#ifdef _WIN32
#ifdef _WIN64
using socket_t = unsigned long long;
#else
using socket_t = unsigned int;
#endif
#else
using socket_t = int;
#endif

namespace ekisocket::ssl {
/**
 * @brief Represents a wrapper for TCP/UDP socket client with optional SSL encryption.
 */
class Client {
public:
    EKISOCKET_EXPORT Client(std::string_view hostname, uint16_t port, bool use_ssl = true, bool use_udp = false);
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    EKISOCKET_EXPORT Client(Client&&) noexcept;
    EKISOCKET_EXPORT Client& operator=(Client&&) noexcept;
    EKISOCKET_EXPORT virtual ~Client();

    [[nodiscard]] EKISOCKET_EXPORT bool connected() const;
    [[nodiscard]] EKISOCKET_EXPORT socket_t socket() const;
    [[nodiscard]] EKISOCKET_EXPORT int timeout() const;

    /**
     * @brief Sets the SSL to either blocking or non-blocking (The SSL Client is originally non-blocking).
     *
     * @param blocking Whether or not the SSL should be blocking.
     */
    EKISOCKET_EXPORT void set_blocking(bool blocking) const;
    EKISOCKET_EXPORT void set_hostname(std::string hostname) const;
    EKISOCKET_EXPORT void set_port(uint16_t port) const;
    EKISOCKET_EXPORT void set_timeout(int milliseconds) const;
    EKISOCKET_EXPORT void set_use_ssl(bool use_ssl) const;

    /**
     * @brief Whether or not the client should verify server certificates. This is useful if certain servers do not
     * offer a valid certificate, or for testing purposes with self-signed certificates.
     *
     * @param verify Whether or not to verify server certificates.
     */
    EKISOCKET_EXPORT void set_verify_certs(bool verify) const;

    /**
     * @brief Connects to the given hostname and port.
     *
     * @param hostname The hostname to connect to.
     * @param port The port to connect to.
     * @return bool Whether or not the connection was successful.
     */
    EKISOCKET_EXPORT bool connect() const;

    /**
     * @brief Sends data over to the server.
     *
     * @param message The data to send.
     * @return size_t The number of bytes sent.
     */
    EKISOCKET_EXPORT size_t send(std::string_view message) const;

    /**
     * @brief Receives data from the server.
     *
     * @return std::string The received data.
     */
    [[nodiscard]] EKISOCKET_EXPORT std::string receive(size_t buf_size = 4096) const;

    /**
     * @brief Calls poll() on the underlying socket to query for the availability of read/write states.
     *
     * @param read Whether or not to query for read availability.
     * @param write Whether or not to query for write availability.
     *
     * @return bool Whether or not the socket is ready for read/write.
     */
    [[nodiscard]] EKISOCKET_EXPORT bool query(bool want_read = false, bool want_write = false) const;

    /**
     * @brief Closes the connection to the server.
     */
    EKISOCKET_EXPORT void close() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl {};
};
} // namespace ekisocket::ssl

#include <atomic>
#include <ekisocket/Socket.hpp>
#include <ekisocket/SslClient.hpp>
#include <ekisocket/Util.hpp>
#include <fmt/format.h>
#include <mutex>
#include <optional>
#include <span>

#ifdef _WIN32
#include <shlwapi.h>
#include <wincrypt.h>
#endif
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#endif
#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

namespace {
template <typename T> struct DeleterOf;

template <> struct DeleterOf<BIO> {
    void operator()(BIO* p) const { BIO_free_all(p); }
};

template <> struct DeleterOf<SSL_CTX> {
    void operator()(SSL_CTX* p) const { SSL_CTX_free(p); }
};

template <typename OpenSSLType> using UniqueSSLPtr = std::unique_ptr<OpenSSLType, DeleterOf<OpenSSLType>>;

UniqueSSLPtr<BIO> operator|(UniqueSSLPtr<BIO>& lower, UniqueSSLPtr<BIO>&& upper)
{
    BIO_push(upper.get(), lower.release());
    return std::move(upper);
}

std::string get_errno_string()
{
    std::string ret {};
#ifdef _WIN32
    ret.resize(256);
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, socketerrno,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ret.data(), static_cast<DWORD>(ret.size()), nullptr);

    if (ret.empty()) {
        ret = std::to_string(socketerrno);
    }
    ret = fmt::format("{}: {}", socketerrno, ret);
#else
    ret = fmt::format("{}: {}", socketerrno, std::strerror(socketerrno));
#endif
    return ret;
}

[[noreturn]] void print_errors_and_throw(std::string_view message, bool use_ssl, bool print_errno = true)
{
    const auto bio = UniqueSSLPtr<BIO>(BIO_new(BIO_s_mem()));
    ERR_print_errors(bio.get());
    char* buf {};
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
    const auto len = BIO_get_mem_data(bio.get(), &buf);
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif
    std::string err_str(buf, static_cast<size_t>(len));
    const auto combined_msg = fmt::format("{}\n"
                                          "{}"
                                          "{}",
        message, use_ssl ? fmt::format("OpenSSL Error: {}\n", err_str) : "",
        print_errno ? fmt::format("Socket Error: {}\n", get_errno_string()) : "");

    throw ekisocket::errors::SslClientError(combined_msg);
}

SSL* get_ssl(BIO* bio)
{
    SSL* ssl {};
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
    BIO_get_ssl(bio, &ssl);
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif
    if (ssl == nullptr) {
        print_errors_and_throw("Unable to get SSL object from BIO.", true);
    }
    return ssl;
}

#ifdef _WIN32
bool load_windows_certificates(const SSL_CTX* ssl)
{
    DWORD flags = CERT_STORE_READONLY_FLAG | CERT_STORE_OPEN_EXISTING_FLAG | CERT_SYSTEM_STORE_CURRENT_USER;
    auto* system_store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, flags, L"Root");

    if (system_store == nullptr) {
        return false;
    }

    PCCERT_CONTEXT it {};
    auto* ssl_store = SSL_CTX_get_cert_store(ssl);

    uint32_t count {};
    while ((it = CertEnumCertificatesInStore(system_store, it)) != nullptr) {
        auto* x509 = d2i_X509(
            nullptr, const_cast<const unsigned char**>(&it->pbCertEncoded), static_cast<int32_t>(it->cbCertEncoded));
        if (x509 != nullptr) {
            if (X509_STORE_add_cert(ssl_store, x509) == 1) {
                ++count;
            }
            X509_free(x509);
        }
    }

    CertFreeCertificateContext(it);
    CertCloseStore(system_store, 0);

    return count != 0;
}
#endif

void verify_the_certificate(SSL const* ssl, [[maybe_unused]] std::string_view expected_hostname)
{
    if (const auto err = SSL_get_verify_result(ssl); err != X509_V_OK) {
        const char* message = X509_verify_cert_error_string(err);
        print_errors_and_throw(message, true);
    }
    if (const auto* cert = SSL_get_peer_certificate(ssl); cert == nullptr) {
        print_errors_and_throw("No certificate was presented by the server.", true);
    }
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    if (X509_check_host(cert, expected_hostname.data(), expected_hostname.size(), 0, nullptr) != 1) {
        print_errors_and_throw("Certificate verification error: X509_check_host failed.", true);
    }
#else
    // X509_check_host is called automatically during verification,
    // because we set it up in main().
#endif
}

template <class T, class U> constexpr bool cmp_equal(T t, U u) noexcept
{
    using UT = std::make_unsigned_t<T>;
    using UU = std::make_unsigned_t<U>;
    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>) {
        return t == u;
    } else if constexpr (std::is_signed_v<T>) {
        return t < 0 ? false : UT(t) == u;
    } else {
        return u < 0 ? false : t == UU(u);
    }
}
} // namespace

namespace ekisocket::ssl {
struct SSLContext {
    UniqueSSLPtr<SSL_CTX> ctx {};
    UniqueSSLPtr<BIO> bio {};
    std::atomic<socket_t> sfd { INVALID_SOCKET };
};

struct Client::Impl {
    Impl(std::string_view hostname, uint16_t port, bool use_ssl = true, bool use_udp = false)
        : m_hostname { hostname }
        , m_port { port }
        , m_use_ssl { use_ssl }
        , m_use_udp { use_udp }
    {
        // Initialization should only be done once.
        std::call_once(ssl_init, initialize_ssl);
        // Increment the ssl client counter.
        ++ssl_client_count;
#ifndef _WIN32
        // Suppressing SIGPIPE.
        struct sigaction sa { };
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGPIPE, &sa, nullptr);
        sigset_t blocked_sig;
        sigemptyset(&blocked_sig);
        sigaddset(&blocked_sig, SIGPIPE);
        pthread_sigmask(SIG_BLOCK, &blocked_sig, nullptr);
#endif
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    ~Impl()
    {
        try {
            close();
        } catch (const errors::SslClientError& e) {
            fmt::print(stderr, "Error closing SslClient connection: {}\n", e.what());
        }
        // When there are no more SSL instances, we must shutdown winsock if using windows.
        if (--ssl_client_count == 0) {
            BIO_sock_cleanup();
        }
    }

    [[nodiscard]] bool connected() const
    {
        std::scoped_lock lk { m_mtx };
        return m_connected;
    }

    [[nodiscard]] socket_t socket() const
    {
        std::scoped_lock lk { m_mtx };
        return m_context.sfd.load();
    }

    [[nodiscard]] int timeout() const
    {
        std::scoped_lock lk { m_mtx };
        return m_timeout.load();
    }

    void set_blocking(bool blocking) { m_timeout.store(blocking ? -1 : 0); }

    void set_hostname(std::string hostname)
    {
        std::scoped_lock lk { m_mtx };
        m_hostname = std::move(hostname);
    }

    void set_port(uint16_t port)
    {
        std::scoped_lock lk { m_mtx };
        m_port = port;
    }

    void set_timeout(int milliseconds)
    {
        std::scoped_lock lk { m_mtx };
        m_timeout.store(milliseconds);
    }

    void set_use_ssl(bool use_ssl)
    {
        std::scoped_lock lk { m_mtx };
        m_use_ssl = use_ssl;
    }

    void set_verify_certs(bool verify)
    {
        std::scoped_lock lk { m_mtx };
        m_verify_certs = verify;
    }

    bool connect()
    {
        std::scoped_lock lk { m_mtx };
        // Check the obvious case of none provided.
        if (m_hostname.empty() || m_port == 0 || m_connected) {
            return false;
        }

        BIO_ADDRINFO* res {};

        if (BIO_lookup_ex(m_hostname.c_str(), std::to_string(m_port).c_str(), BIO_LOOKUP_CLIENT, AF_INET,
                m_use_udp ? SOCK_DGRAM : SOCK_STREAM, m_use_udp ? IPPROTO_UDP : IPPROTO_TCP, &res)
            == 0) {
            print_errors_and_throw("Unable to lookup address.", m_use_ssl);
        }
        for (const BIO_ADDRINFO* ai = res; ai != nullptr; ai = BIO_ADDRINFO_next(ai)) {
            const auto sfd
                = BIO_socket(BIO_ADDRINFO_family(ai), BIO_ADDRINFO_socktype(ai), BIO_ADDRINFO_protocol(ai), 0);

            if (cmp_equal(sfd, INVALID_SOCKET)) {
                continue;
            }

// With non-blocking sockets, we want our expected return type.
#ifdef _WIN32
#define SOCKET_ERRNO_CONDITION (socketerrno == WSAEWOULDBLOCK)
#else
#define SOCKET_ERRNO_CONDITION (socketerrno == EINPROGRESS || socketerrno == EWOULDBLOCK)
#endif
            if (BIO_connect(
                    sfd, BIO_ADDRINFO_address(ai), m_use_udp ? BIO_SOCK_NONBLOCK : BIO_SOCK_NODELAY | BIO_SOCK_NONBLOCK)
                    == 0
                && !SOCKET_ERRNO_CONDITION && socketerrno != 0) {
                BIO_closesocket(sfd);
                continue;
            }
            if (cmp_equal(sfd, INVALID_SOCKET)) {
                print_errors_and_throw("Unable to connect to host.", m_use_ssl);
            }

            m_context.bio = UniqueSSLPtr<BIO>(BIO_new_socket(sfd, BIO_CLOSE));
            m_context.sfd.store(sfd);
            BIO_ADDRINFO_free(res);
            break;
        }
        if (!m_context.bio) {
            print_errors_and_throw("Error creating BIO.", m_use_ssl);
        }

        const auto old_timeout = m_timeout.load();

        if (!m_use_udp) {
            m_timeout.store(-1);
            // We want to perform a select on the socket since we're not sure if the connection is ready.
            m_connected = query(false, true);
            m_timeout.store(old_timeout);

            // Get the getsockopt for the socket to check if the connection is ready.
#ifdef _WIN32
            int optlen = sizeof(int);
            int optval {};
#define OPTVAL (reinterpret_cast<char*>(&optval))
#else
            socklen_t optlen = sizeof(socket_t);
            socket_t optval {};
#define OPTVAL (&optval)
#endif
            if (getsockopt(m_context.sfd.load(), SOL_SOCKET, SO_ERROR, OPTVAL, &optlen) == -1) {
                print_errors_and_throw("Unable to get socket options.", m_use_ssl);
            }
            if (optval != 0) {
                std::string so_err_str {};
#ifdef _WIN32
                std::string buffer(94, '\0');
                (void)strerror_s(buffer.data(), buffer.size(), optval);
                so_err_str = buffer;
#else
                so_err_str = strerror(optval);
#endif
                print_errors_and_throw(fmt::format("Socket Error {}: {}", optval, so_err_str), m_use_ssl, false);
            }
        } else {
            m_connected = true;
        }

        if (m_use_ssl) {
            create_context();
            if (auto ret = BIO_do_connect(m_context.bio.get()); ret <= 0) {
                if (!BIO_should_retry(m_context.bio.get())) {
                    print_errors_and_throw("Unable to connect to host.", m_use_ssl);
                }
                do {
                    ret = BIO_do_connect(m_context.bio.get());
                } while (ret != 1);
            }
            if (m_verify_certs) {
                verify_the_certificate(get_ssl(m_context.bio.get()), m_hostname);
            }
        }
        return true;
    }

    size_t send(std::string_view message)
    {
        // If the length of our message is greater than the max value of an int, we cannot guarantee that we can send it
        // all.
        if (message.length() > static_cast<size_t>((std::numeric_limits<int>::max)())) {
            throw errors::SslClientError("Message too long to send. Please split it into smaller messages.");
        }
        if (!m_connected) {
            print_errors_and_throw("Not connected.", m_use_ssl);
        }
        if (!query(false, true)) {
            return 0;
        }

        const auto ret = BIO_write(m_context.bio.get(), message.data(), static_cast<int>(message.length()));

        if (ret <= 0) {
            if (BIO_should_retry(m_context.bio.get())) {
                return 0;
            }
            m_connected = false;
        }
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif
        BIO_flush(m_context.bio.get());
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif
        return static_cast<size_t>(ret);
    }

    std::string receive(size_t buf_size = 4096)
    {
        if (buf_size > static_cast<size_t>((std::numeric_limits<int>::max)())) {
            throw errors::SslClientError("Buffer size too large to receive. Please split it into smaller buffers.");
        }
        if (!m_connected) {
            print_errors_and_throw("Not connected.", m_use_ssl);
        }
        if (!m_context.bio) {
            print_errors_and_throw("Could not retrieve the underlying socket BIO.", m_use_ssl);
        }

        size_t bytes_read {};
        std::string ret(buf_size, '\0');

        // Check if we have any pending data left in our BIO's read buffer.
        if (const auto pending = BIO_ctrl_pending(m_context.bio.get()); pending > 0) {
            bytes_read += BIO_read(m_context.bio.get(), ret.data(), static_cast<int>((std::min)(buf_size, pending)));
        }
        // Reads of 0 should still be allowed for disconnect discovery.
        if (bytes_read > 0 && bytes_read == buf_size) {
            return ret;
        }

        (void)query(true, false);

        const auto len = BIO_read(
            m_context.bio.get(), std::span(ret).subspan(bytes_read).data(), static_cast<int>(buf_size - bytes_read));

        bytes_read += (std::max)(len, 0);
        ret.resize(bytes_read);
        ret.shrink_to_fit();

        if (len == 0 && !BIO_should_retry(m_context.bio.get())) {
            m_connected = false;
            return ret;
        }
        if (len <= 0) {
            if (BIO_should_retry(m_context.bio.get())) {
                return ret;
            }
            print_errors_and_throw("Error receiving data.", m_use_ssl);
        }

        return ret;
    }

    [[nodiscard]] bool query(bool want_read = false, bool want_write = false) const
    {
        if (!m_context.bio || m_context.sfd.load() == INVALID_SOCKET) {
            return false;
        }

        const auto sfd = m_context.sfd.load();
        pollfd pfd { .fd = sfd, .events = 0, .revents = 0 };

        if (want_read) {
            pfd.events |= POLLIN;
        }
        if (want_write) {
            pfd.events |= POLLOUT;
        }
        if (const auto ret = ::poll(&pfd, 1, m_timeout.load()); ret <= 0) {
            return false;
        }

        return (want_read == static_cast<bool>(pfd.revents & POLLIN))
            && (want_write == static_cast<bool>(pfd.revents & POLLOUT))
            && !static_cast<bool>(pfd.revents & (POLLNVAL | POLLERR | POLLHUP));
    }

    void close()
    {
        std::scoped_lock lk { m_mtx };
        if (!m_connected) {
            return;
        }
        // If the client is TCP, we need to gracefully close the connection.
        if (!m_use_udp) {
            auto sfd = m_context.sfd.load();
#ifdef _WIN32
            WSAEVENT event = WSACreateEvent();
            WSAEventSelect(sfd, event, FD_CLOSE);
            shutdown(sfd, SD_SEND);
            WaitForSingleObject(event, INFINITE);
#else
            shutdown(sfd, SHUT_WR);
#endif
            const auto old_timeout = m_timeout.load();
            set_blocking(false);

            while (m_connected) {
                receive();
            }
#ifdef _WIN32
            WSACloseEvent(event);
#endif
            m_timeout.store(old_timeout);
        }

        // Because we essentially closed the connection, we do not need the BIO to close it for us.
        auto* bio = m_context.bio.get();
        while (bio != nullptr) {
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-value"
#endif
            BIO_set_close(bio, BIO_NOCLOSE);
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

            bio = BIO_next(bio);
        }

        m_context.bio = nullptr;
        m_context.ctx = nullptr;
        m_context.sfd.store(INVALID_SOCKET);
        m_connected = false;
    }

private:
    static void initialize_ssl()
    {
#ifndef _WIN32
        // Suppressing SIGPIPE.
        struct sigaction sa { };
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGPIPE, &sa, nullptr);
        sigset_t blocked_sig;
        sigemptyset(&blocked_sig);
        sigaddset(&blocked_sig, SIGPIPE);
        pthread_sigmask(SIG_BLOCK, &blocked_sig, nullptr);
#endif
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        SSL_library_init();
        SSL_load_error_strings();
#else
        OPENSSL_init_ssl(0, nullptr);
#endif
    }

    /// Flag used for SSL initialization.
    static std::once_flag ssl_init;
    /// Counter used for destroying winsock when all Client objects are destroyed.
    static std::atomic_uint32_t ssl_client_count;

    /**
     * @brief Creates a new SSL context, which contains data that is essential for establishing an SSL/TLS connection.
     */
    void create_context()
    {
        UniqueSSLPtr<SSL_CTX> ctx {};

        if (m_use_udp) {
            ctx = UniqueSSLPtr<SSL_CTX>(SSL_CTX_new(DTLS_client_method()));
        } else {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
            ctx = UniqueSSLPtr<SSL_CTX>(SSL_CTX_new(SSLv23_client_method()));
#else
            ctx = UniqueSSLPtr<SSL_CTX>(SSL_CTX_new(TLS_client_method()));
#endif
        }
        if (!ctx) {
            print_errors_and_throw("Unable to create SSL context.", true);
        }
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
        // Set minimum TLS version.
        if (!SSL_CTX_set_min_proto_version(ctx.get(), m_use_udp ? DTLS1_2_VERSION : TLS1_2_VERSION)) {
            print_errors_and_throw("Unable to set minimum TLS version.", true);
        }
#endif
        // Get certificates from the system store.
        bool loaded_certs {};
#ifdef _WIN32
        loaded_certs = load_windows_certificates(ctx.get());
#else
        loaded_certs = SSL_CTX_set_default_verify_paths(ctx.get()) == 1;
#endif
        if (!loaded_certs) {
            print_errors_and_throw("Unable to load certificates from the system store.", true);
        }

        m_context.ctx = std::move(ctx);

        // Create a new SSL object.
        m_context.bio = m_context.bio | UniqueSSLPtr<BIO>(BIO_new_ssl(m_context.ctx.get(), 1));
        // Disabling retries.
        SSL_set_mode(get_ssl(m_context.bio.get()), SSL_MODE_AUTO_RETRY);
// Server Name Indication.
#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
        SSL_set_tlsext_host_name(get_ssl(m_context.bio.get()), m_hostname.c_str());
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif
        // Verifying the host is on the certificate.
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
        SSL_set1_host(get_ssl(m_context.bio.get()), m_hostname.c_str());
#endif
    }

    /// Mutex used for thread safety.
    mutable std::mutex m_mtx {};
    /// The hostname of the server.
    std::string m_hostname {};
    /// The port of the server.
    uint16_t m_port {};
    /// Whether or not ssl is enabled.
    bool m_use_ssl {};
    /// Whether or not the client should be using the UDP protocol.
    bool m_use_udp {};
    /// Whether or not the client is connected to the server.
    bool m_connected {};
    /// The underlying SSL context.
    SSLContext m_context {};
    /// Whether or not the client should verify server certificates.
    bool m_verify_certs {};
    /// The amount of time to wait for a socket to become ready for read/write. Defaults to -1, which means blocking.
    std::atomic_int m_timeout { -1 };
};

std::once_flag Client::Impl::ssl_init {};
std::atomic_uint32_t Client::Impl::ssl_client_count {};

Client::Client(std::string_view hostname, uint16_t port, bool use_ssl, bool use_udp)
    : m_impl { std::make_unique<Client::Impl>(hostname, port, use_ssl, use_udp) }
{
}
Client::Client(Client&&) noexcept = default;
Client& Client::operator=(Client&&) noexcept = default;
Client::~Client() = default;

bool Client::connected() const { return m_impl->connected(); }

socket_t Client::socket() const { return m_impl->socket(); }

int Client::timeout() const { return m_impl->timeout(); }

void Client::set_blocking(bool blocking) const { return m_impl->set_blocking(blocking); }

void Client::set_hostname(std::string hostname) const { m_impl->set_hostname(std::move(hostname)); }

void Client::set_port(uint16_t port) const { m_impl->set_port(port); }

void Client::set_timeout(int milliseconds) const { return m_impl->set_timeout(milliseconds); }

void Client::set_use_ssl(bool use_ssl) const { m_impl->set_use_ssl(use_ssl); }

void Client::set_verify_certs(bool verify) const { return m_impl->set_verify_certs(verify); }

bool Client::connect() const { return m_impl->connect(); }

size_t Client::send(std::string_view message) const { return m_impl->send(message); }

std::string Client::receive(size_t buf_size) const { return m_impl->receive(buf_size); }

bool Client::query(bool want_read, bool want_write) const { return m_impl->query(want_read, want_write); }

void Client::close() const { return m_impl->close(); }

} // namespace ekisocket::ssl

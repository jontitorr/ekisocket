#pragma once
#include <ekisocket/SslClient.hpp>
#include <ekisocket/Uri.hpp>
#include <functional>

namespace ekisocket {
namespace ssl {
    class Client;
} // namespace ssl

namespace ws {
    class Client;
} // namespace ws

namespace http {
    /// All the HTTP methods.
    enum class Method : uint8_t { GET, POST, PUT, DELETE_, HEAD, OPTIONS, CONNECT, TRACE, PATCH };

    using Headers = util::CaseInsensitiveMap;

    /// Generic HTTP Response.
    struct Response {
        uint16_t status_code {};
        std::string status_message {};
        Headers headers {};
        std::string body {};
    };

    /// Callback for the streamed body.
    using BodyCallback = std::function<void(std::string_view)>;

    /**
     * @brief Represents a client that can perform HTTP(S) requests. Really is just a collection of functions that are
     * used to perform HTTP(S) requests.
     */
    class Client {
    public:
        EKISOCKET_EXPORT explicit Client();
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        EKISOCKET_EXPORT Client(Client&&) noexcept;
        EKISOCKET_EXPORT Client& operator=(Client&&) noexcept;
        EKISOCKET_EXPORT virtual ~Client();

        [[nodiscard]] EKISOCKET_EXPORT Response get(std::string_view, const Headers& headers = {},
            std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
        [[nodiscard]] EKISOCKET_EXPORT Response post(std::string_view, const Headers& headers = {},
            std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
        [[nodiscard]] EKISOCKET_EXPORT Response put(std::string_view, const Headers& headers = {},
            std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
        [[nodiscard]] EKISOCKET_EXPORT Response delete_(std::string_view, const Headers& headers = {},
            std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
        [[nodiscard]] EKISOCKET_EXPORT Response head(std::string_view, const Headers& headers = {},
            std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
        [[nodiscard]] EKISOCKET_EXPORT Response options(std::string_view, const Headers& headers = {},
            std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
        [[nodiscard]] EKISOCKET_EXPORT Response connect(std::string_view, const Headers& headers = {},
            std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
        [[nodiscard]] EKISOCKET_EXPORT Response trace(std::string_view, const Headers& headers = {},
            std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
        [[nodiscard]] EKISOCKET_EXPORT Response patch(std::string_view, const Headers& headers = {},
            std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});

        /**
         * @brief Sends an HTTP Request to a server, taking into consideration standard HTTP practices such as headers
         * and content-length, which allows us to continuously receive until the complete data has been received.
         *
         * @param method The HTTP Method to use.
         * @param url The URL to send the request to.
         * @param headers The headers to send with the request.
         * @param body The body of the request.
         * @param keep_alive  Whether or not to keep the connection alive.
         * @param stream Whether or not to stream the response.
         * @param cb The callback to call for each chunk of data received.
         *
         * @return Response The response from the server.
         */
        [[nodiscard]] EKISOCKET_EXPORT Response request(const Method& method, std::string_view url,
            const Headers& headers, std::string_view body, bool keep_alive = false, bool stream = false,
            const BodyCallback& cb = {}) const;

    private:
        friend ws::Client;

        [[nodiscard]] ssl::Client& ssl() const;

        struct Impl;
        std::unique_ptr<Impl> m_impl {};
    };

    [[nodiscard]] EKISOCKET_EXPORT Response get(std::string_view url, const Headers& headers = {},
        std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
    [[nodiscard]] EKISOCKET_EXPORT Response post(std::string_view url, const Headers& headers = {},
        std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
    [[nodiscard]] EKISOCKET_EXPORT Response put(std::string_view url, const Headers& headers = {},
        std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
    [[nodiscard]] EKISOCKET_EXPORT Response delete_(std::string_view url, const Headers& headers = {},
        std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
    [[nodiscard]] EKISOCKET_EXPORT Response head(std::string_view url, const Headers& headers = {},
        std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
    [[nodiscard]] EKISOCKET_EXPORT Response options(std::string_view url, const Headers& headers = {},
        std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
    [[nodiscard]] EKISOCKET_EXPORT Response connect(std::string_view url, const Headers& headers = {},
        std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
    [[nodiscard]] EKISOCKET_EXPORT Response trace(std::string_view url, const Headers& headers = {},
        std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
    [[nodiscard]] EKISOCKET_EXPORT Response patch(std::string_view url, const Headers& headers = {},
        std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});

    /**
     * @brief Sends an HTTP Request to a server, specifying the HTTP method, URL, headers, and body. If you are using a
     * specific HTTP method, you should consider using those instead of this function.
     *
     * @param method The HTTP Method to use.
     * @param url The URL to send the request to.
     * @param headers The headers to send with the request.
     * @param body The body of the request.
     * @return Response The response from the server.
     */
    [[nodiscard]] EKISOCKET_EXPORT Response request(const Method& method, std::string_view url,
        const Headers& headers = {}, std::string_view body = {}, bool stream = false, const BodyCallback& cb = {});
} // namespace http
} // namespace ekisocket

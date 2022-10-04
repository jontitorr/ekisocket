#pragma once
#include <ekisocket/SSLClient.hpp>
#include <ekisocket/URI.hpp>
#include <functional>

#define DECLARE_HTTP_METHOD(name, method)                                                                              \
    [[nodiscard]] EKISOCKET_EXPORT Response name(const std::string& url, const Headers& headers = {},                  \
        const std::string& body = {}, bool stream = false, const BodyCallback& cb = {});

namespace ekisocket {
namespace ssl {
    class Client;
}
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

        DECLARE_HTTP_METHOD(get, GET)
        DECLARE_HTTP_METHOD(post, POST)
        DECLARE_HTTP_METHOD(put, PUT)
        DECLARE_HTTP_METHOD(delete_, DELETE_)
        DECLARE_HTTP_METHOD(head, HEAD)
        DECLARE_HTTP_METHOD(options, OPTIONS)
        DECLARE_HTTP_METHOD(connect, CONNECT)
        DECLARE_HTTP_METHOD(trace, TRACE)
        DECLARE_HTTP_METHOD(patch, PATCH)

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
        [[nodiscard]] EKISOCKET_EXPORT Response request(const Method& method, const std::string& url,
            const Headers& headers, std::string_view body, bool keep_alive = false, bool stream = false,
            const BodyCallback& cb = {}) const;

    private:
        friend ws::Client;

        [[nodiscard]] ssl::Client& ssl() const;

        struct Impl;
        std::unique_ptr<Impl> m_impl {};
    };

#define DECLARE_OUTSIDE_FUNCTION(name)                                                                                 \
    [[nodiscard]] EKISOCKET_EXPORT Response name(const std::string& url, const Headers& headers = {},                  \
        const std::string& body = {}, bool stream = false, const BodyCallback& cb = {});

    DECLARE_OUTSIDE_FUNCTION(get)
    DECLARE_OUTSIDE_FUNCTION(post)
    DECLARE_OUTSIDE_FUNCTION(put)
    DECLARE_OUTSIDE_FUNCTION(delete_)
    DECLARE_OUTSIDE_FUNCTION(head)
    DECLARE_OUTSIDE_FUNCTION(options)
    DECLARE_OUTSIDE_FUNCTION(connect)
    DECLARE_OUTSIDE_FUNCTION(trace)
    DECLARE_OUTSIDE_FUNCTION(patch)

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
    [[nodiscard]] EKISOCKET_EXPORT Response request(const Method& method, const std::string& url,
        const Headers& headers = {}, const std::string& body = {}, bool stream = false, const BodyCallback& cb = {});
} // namespace http
} // namespace ekisocket

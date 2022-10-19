#pragma once
#include <stdexcept>

namespace ekisocket::errors {
struct HttpClientError : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct SslClientError : std::runtime_error {
    using runtime_error::runtime_error;
};

struct WebSocketClientError : std::runtime_error {
    using runtime_error::runtime_error;
};
}

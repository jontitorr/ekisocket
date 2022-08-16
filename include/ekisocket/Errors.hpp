#pragma once
#include <stdexcept>

namespace ekisocket::errors {
struct HTTPClientError : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct SSLClientError : std::runtime_error {
    using runtime_error::runtime_error;
};

struct WebSocketClientError : std::runtime_error {
    using runtime_error::runtime_error;
};
}

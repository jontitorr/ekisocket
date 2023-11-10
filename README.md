# Ekisocket

**This repository has been archived. The development has been succeeded/continued in** <https://github.com/xminent/net>.

A small network library written for use with my library [Ekizu](https://github.com/xminent/ekizu)

## Features

- ssl::Client: A TCP/UDP Client with optional SSL support.
- http::Client: An HTTP(S) client.
- ws::Client: A WebSocket client.

## Getting Started

### Prerequisites

- [CMake](https://cmake.org/download/) (version >= 3.15)
- Compiler with C++20 support, i.e. MSVC, GCC, Clang

### Installing

This library uses [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) to manage dependencies. It is an amazing package manager for CMake projects and allows us to install the entire library using the following commands:

```bash
  git clone https://www.github.com/xminent/ekisocket
  cd ekisocket
  make install
```

From there you can simply integrate it into your CMake project like so:

```cmake
    find_package(ekisocket REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ekisocket::ekisocket)
```

## Usage/Examples

### ssl::Client

```cpp
#include <ekisocket/SslClient.hpp>
#include <iostream>

int main()
{
    // Access a TCP server found in localhost:8080.
    ekisocket::ssl::Client client("localhost", 8080, /* use_ssl */ false, /* use_udp = false */);
    client.connect();
    client.send("Hello World!");
    // Gracefully closes on destruction.
}
```

### http::Client

```cpp
#include <ekisocket/HttpClient.hpp>
#include <iostream>

int main()
{
    // Simple GET request to an API endpoint.
    const auto res = ekisocket::http::get("https://catfact.ninja/fact");
    std::cout << res.body << '\n';
}
```

### ws::Client

```cpp
#include <ekisocket/WebSocketClient.hpp>
#include <iostream>

int main()
{
    // Simple GET request to an API endpoint.
    ekisocket::ws::Client client("wss://gateway.discord.gg/?v=10&encoding=json");
    client.set_on_message([](const ekisocket::ws::Message& msg){
        std::cout << "Received message: " << msg.data << '\n';
    });
    // Blocks until disconnected. (Automatic reconnect is on by default).
    client.start();
    // Automatic reconnect can be turned off by doing:
    // client.set_automatic_reconnect(false);
    // For unblocking, run:
    // client.start_async();
}
```

## Documentation

[Documentation](https://xminent.github.io/ekisocket)

## Dependencies

### Third party Dependencies

- [OpenSSL](https://openssl.org/) (comes bundled with project, unless you have it installed)
- [fmt](https://github.com/fmtlib/fmt) (comes bundled with project, unless you have it installed)

## License

[MIT](https://choosealicense.com/licenses/mit/)

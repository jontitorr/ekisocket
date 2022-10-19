#include <ekisocket/SslClient.hpp>
#include <iostream>

int main()
{
    // Sending a raw HTTP request to a server.
    ekisocket::ssl::Client client("google.com", 443);
    client.set_blocking(true);
    client.connect();

    // Send a GET request to the server.
    const std::string_view to_send { "GET / HTTP/1.1\r\nHost: google.com\r\n\r\n" };

    const auto sent = client.send(to_send);

    std::cout << "Sent " << sent << " bytes.\n";

    // Receive the response.
    const auto res = client.receive();

    std::cout << res << '\n';

    // Automatically closes the connection on destruction.
}

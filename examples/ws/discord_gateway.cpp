#include <ekisocket/WebSocketClient.hpp>
#include <iostream>

int main()
{
    // Simple GET request to an API endpoint.
    ekisocket::ws::Client client("wss://gateway.discord.gg/?v=10&encoding=json");
    client.set_on_message(
        [](const ekisocket::ws::Message& msg) { std::cout << "Received message: " << msg.data << '\n'; });
    // Blocks until disconnected. (Automatic reconnect is on by default).
    client.start();
    // Automatic reconnect can be turned off by doing:
    // client.set_automatic_reconnect(false);
    // For unblocking, run:
    // client.start_async();
}

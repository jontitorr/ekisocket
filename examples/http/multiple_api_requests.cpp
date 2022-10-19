#include <ekisocket/HttpClient.hpp>
#include <iostream>

int main()
{
    // Utilizing the same client and making multiple requests.
    const std::string_view url = "https://catfact.ninja/fact";

    ekisocket::http::Client client {};

    for (int i = 0; i < 10; ++i) {
        const auto res = client.get(url);
        std::cout << res.body << '\n';
    }
}

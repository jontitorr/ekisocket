#include <ekisocket/HttpClient.hpp>
#include <iostream>

int main()
{
    // Simple GET request to an API endpoint.
    const auto res = ekisocket::http::get("https://catfact.ninja/fact");
    std::cout << res.body << '\n';
}

#include <algorithm>
#include <ekisocket/URI.hpp>
#include <fmt/format.h>
#include <optional>

namespace ekisocket::http {
URI URI::parse(const std::string& uri)
{
    URI u {};
    if (!u.parse_scheme(uri)) {
        return u;
    }

    const auto& scheme = u.scheme;
    // Search for the end of the path, which ends either on the start of a query or a fragment.
    auto path_end = uri.find_first_of("?#", scheme.length() + 1);
    auto authority_and_path = uri.substr(scheme.length() + 1, path_end - (scheme.length() + 1));
    auto query_and_or_fragment = uri.substr(authority_and_path.length() + scheme.length() + 1);

    // Split off the authority marker if it is still present.
    if (authority_and_path.starts_with("//")) {
        // Remove the authority marker.
        authority_and_path.erase(0, 2);
        // Separate authority from path.
        auto authority_end = authority_and_path.find('/');
        if (authority_end == std::string::npos) {
            authority_end = authority_and_path.length();
        }
        auto authority = authority_and_path.substr(0, authority_end);
        // Parse the authority.
        u.parse_authority(authority);
        u.path = authority_and_path.substr(authority.length());
    }

    // Split the query and/or fragment.
    auto fragment_end = query_and_or_fragment.find('#');
    if (fragment_end == std::string::npos) {
        fragment_end = query_and_or_fragment.length();
    }

    u.query = query_and_or_fragment.substr(0, fragment_end);

    // Remove '?' if present
    if (u.query.starts_with('?')) {
        u.query.erase(0, 1);
    }

    u.fragment = query_and_or_fragment.substr(fragment_end);

    // Remove '#' if present
    if (u.fragment.starts_with('#')) {
        u.fragment.erase(0, 1);
    }
    return u;
}

void URI::add_query_parameter(const std::string& key, const std::string& value)
{
    if (!query.empty()) {
        query += '&';
    }
    query += fmt::format("{}={}", key, value);
}

void URI::add_query_parameters(const QueryParams& params)
{
    for (const auto& [key, value] : params) {
        add_query_parameter(key, value);
    }
}

std::string URI::to_string() const
{
    std::string str = fmt::format("{}://", scheme);

    if (!username.empty()) {
        str += username;
        if (!password.empty()) {
            str += fmt::format(":{}", password);
        }
        str += "@";
    }

    str += host;

    if (port != 0) {
        str += fmt::format(":{}", port);
    }

    str += path;

    if (!query.empty()) {
        str += fmt::format("?{}", query);
    }
    if (!fragment.empty()) {
        str += fmt::format("#{}", fragment);
    }
    return str;
}

bool URI::parse_scheme(const std::string& str)
{
    // Look for a '/', since searching for a colon can be found in other places besides the scheme.
    auto hierachical_start = str.find('/');

    if (hierachical_start == std::string::npos) {
        hierachical_start = str.length();
    }

    // The scheme ends until the first ':'.
    size_t scheme_end {};

    while (scheme_end < hierachical_start) {
        if (str[scheme_end] == ':') {
            break;
        }
        if (scheme_end == (hierachical_start - 1)) {
            return false;
        }
        ++scheme_end;
    }

    scheme = str.substr(0, scheme_end);
    std::transform(
        scheme.begin(), scheme.end(), scheme.begin(), [](uint8_t c) { return static_cast<char>(std::tolower(c)); });
    return true;
}

bool URI::parse_authority(const std::string& str)
{
    // Look for a '@', which indicates the presence of user information.
    std::optional<size_t> user_info_end = str.find('@');

    if (user_info_end == std::string::npos) {
        // No user information. (This is fine as it is optional.)
        user_info_end = std::nullopt;
    } else {
        size_t username_end {};

        for (username_end = 0; username_end < user_info_end; ++username_end) {
            if (str[username_end] == ':') {
                break;
            }
            if (username_end == (user_info_end.value() - 1)) {
                return false;
            }
        }

        username = str.substr(0, username_end);
        password = str.substr(username_end + 1, user_info_end.value() - (username_end + 1));
    }

    std::string port_str {};
    const auto user_info_end_plus = [&user_info_end](size_t n) {
        if (user_info_end.has_value()) {
            return user_info_end.value() + n;
        }
        return static_cast<size_t>(-1 + n);
    };

    if (str[user_info_end_plus(1)] == '[') {
        // Search for the end of the IPv6 address.
        auto ipv6_end = str.find(']', user_info_end_plus(2));
        if (ipv6_end == std::string::npos) {
            return false;
        }

        // Get the IPv6 address.
        host = str.substr(user_info_end_plus(2), ipv6_end - user_info_end_plus(2));
        // Get the port.
        port_str = str.substr(ipv6_end + 1); // is null terminator at worst.
    } else {
        // Search for the end of the hostname.
        auto hostname_end = str.find(':', user_info_end_plus(1));
        if (hostname_end == std::string::npos) {
            hostname_end = str.length();
        }
        // Get the hostname.
        host = str.substr(user_info_end_plus(1), hostname_end - user_info_end_plus(1));
        // Get the port.
        port_str = str.substr(hostname_end); // is null terminator at worst.
    }

    // Remove colon from port if it has one.
    if (port_str.starts_with(':')) {
        port_str.erase(0, 1);
    }

    port = port_str.empty() ? 0 : static_cast<uint16_t>(std::stoul(port_str));
    return true;
}
} // namespace ekisocket::http

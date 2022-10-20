#include <algorithm>
#include <ekisocket/Uri.hpp>
#include <fmt/format.h>

namespace {
inline size_t find_or_else(std::string_view str, char c, size_t default_value)
{
    auto pos = str.find(c);
    return pos == std::string_view::npos ? default_value : pos;
}

inline char get_safe_char(std::string_view str, size_t index, char default_value = '\0')
{
    return index < str.length() ? str[index] : default_value;
}

inline std::pair<std::string_view, std::string_view> split_at(std::string_view str, size_t index)
{
    return { str.substr(0, index), str.substr(index) };
}

std::string to_lowercase(std::string_view str)
{
    std::string ret(str);
    std::transform(ret.begin(), ret.end(), ret.begin(), [](char c) { return static_cast<char>(std::tolower(c)); });
    return ret;
}

void parse_authority(ekisocket::http::Uri& uri_struct, std::string_view authority)
{
    // Look for a '@', which indicates the presence of user information.
    const auto user_info_end = authority.find('@');

    if (user_info_end != std::string::npos) {
        // Split the authority into the user information and the host.
        const auto user_info = authority.substr(0, user_info_end);

        // Split the user information into the username and password.
        const auto username_end = user_info.find(':');

        if (username_end != std::string::npos) {
            uri_struct.username = std::string(user_info.substr(0, username_end));
            uri_struct.password = std::string(user_info.substr(username_end + 1));
        } else {
            uri_struct.username = std::string(user_info);
        }
    }

    const auto user_info_end_plus = user_info_end == std::string::npos ? 0 : user_info_end + 1;
    std::string_view port_str {};

    // If the authority at index user_info_end_plus is a '[' then we have an IPv6 address.
    if (get_safe_char(authority, user_info_end_plus) == '[') {
        // Find the end of the IPv6 address.
        const auto ipv6_end = find_or_else(authority.substr(user_info_end_plus), ']', authority.length());

        uri_struct.host = to_lowercase(authority.substr(user_info_end_plus + 1, ipv6_end - 1));

        // If the authority at index ipv6_end + 1 is a ':' then we have a port.
        if (get_safe_char(authority, user_info_end_plus + ipv6_end + 1) == ':') {
            port_str = authority.substr(user_info_end_plus + ipv6_end + 2);
        }
    } else {
        // Find the end of the host.
        auto host_end = authority.substr(user_info_end_plus).find(':');
        if (host_end == std::string::npos) {
            host_end = authority.length();
        } else {
            host_end += user_info_end_plus;
        }

        uri_struct.host = to_lowercase(authority.substr(user_info_end_plus, host_end - user_info_end_plus));
        port_str = authority.substr(host_end);
    }

    // Remove colon from port string, if present.
    if (port_str.starts_with(':')) {
        port_str.remove_prefix(1);
    }

    uri_struct.port = port_str.empty()
        ? std::nullopt
        : std::optional<uint16_t>(static_cast<uint16_t>(std::stoul(std::string(port_str))));
}
} // namespace

namespace ekisocket::http {
Uri Uri::parse(std::string_view url)
{
    Uri uri_struct {};

    // Parse the scheme.

    const auto scheme_end = ([&uri_struct, &url]() {
        bool scheme_found {};
        size_t ret {};

        const auto is_double_slash
            = [&url](const auto i) { return get_safe_char(url, i) == '/' && get_safe_char(url, i + 1) == '/'; };

        for (size_t i {}; const auto& c : url) {
            if (c == '/') {
                if (is_double_slash(i) && scheme_found) {
                    uri_struct.scheme = to_lowercase(url.substr(0, ret - 1));
                }
                break;
            }
            if (c == ':') {
                scheme_found = true;
                ret = i + 1;

                if (!is_double_slash(i + 1)) {
                    uri_struct.scheme = to_lowercase(url.substr(0, ret - 1));
                    break;
                }
            }
            ++i;
        }

        return ret;
    })();

    // Search for the end of the path, which ends either on the start of a query or a fragment.
    // Look for the path end, by searching for either a '?' or a '#', starting from the index of the scheme's end.

    const auto path_end = ([&url] {
        auto ret = url.find_first_of("?#");
        return ret == std::string::npos ? url.length() : ret;
    })();
    auto authority_and_path = url.substr(scheme_end, path_end - scheme_end);
    const auto query_and_fragment = url.substr(authority_and_path.length() + scheme_end);
    bool has_authority {};

    // Split off the authority marker if it is still present.
    if (authority_and_path.starts_with("//")) {
        has_authority = true;
        authority_and_path.remove_prefix(2);
    }
    if (has_authority) {
        // Look for the end of the authority, which ends on the start of the path.
        const auto authority_end = find_or_else(authority_and_path, '/', authority_and_path.length());

        const auto authority = authority_and_path.substr(0, authority_end);
        parse_authority(uri_struct, authority);
        uri_struct.path = std::string(authority_and_path.substr(authority_end));

    } else {
        // If there is no authority, then the path is the entire string.
        uri_struct.path = std::string(authority_and_path);
    }

    auto [query, fragment]
        = split_at(query_and_fragment, find_or_else(query_and_fragment, '#', query_and_fragment.length()));

    // Because fragment could be empty, we need to check if it is empty.
    if (!fragment.empty()) {
        uri_struct.fragment = std::string(fragment.substr(1));
    }

    // Parse the query.

    // Remove the ? from the query, if it is present.
    if (query.starts_with('?')) {
        query.remove_prefix(1);
    }
    if (!query.empty()) {
        const auto query_parameters = util::split(query, "&");

        for (const auto& query_parameter : query_parameters) {
            const auto [key, value]
                = split_at(query_parameter, find_or_else(query_parameter, '=', query_parameter.length()));

            uri_struct.query.try_emplace(std::string(key), value.starts_with('=') ? value.substr(1) : value);
        }
    }

    return uri_struct;
}
} // namespace ekisocket::http

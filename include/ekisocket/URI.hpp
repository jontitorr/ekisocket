#pragma once
#include <string>

namespace ekisocket::http {
/**
 * @brief Represents a Uniform Resource Identifier commonly used in all HTTP(S) requests.
 */
struct URI {
    std::string scheme {};
    std::string username {};
    std::string password {};
    std::string host {};
    uint16_t port {};
    std::string path {};
    std::string query {};
    std::string fragment {};

    /**
     * @brief Returns a URI object parsed from the given string.
     *
     * @param uri The string to parse.
     * @return URI The parsed URI.
     */
    [[nodiscard]] static URI parse(const std::string& uri);

    // function that converts the URI to a string
    [[nodiscard]] std::string to_string() const;

private:
    /**
     * @brief Parses the scheme from the given URI string.
     *
     * @param str The string to parse.
     * @return bool Whether or not the parsing was successful.
     */
    bool parse_scheme(const std::string& str);
    /**
     * @brief Parses the authority from the given URI string.
     *
     * @param str The string to parse.
     * @return bool Whether or not the parsing was successful.
     */
    bool parse_authority(const std::string& str);
};
} // namespace ekisocket::http

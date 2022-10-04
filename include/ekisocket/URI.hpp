#pragma once
#include <ekisocket/Util.hpp>
#include <ekisocket_export.h>
#include <map>
#include <string>

namespace ekisocket::http {
/**
 * @brief Represents a Uniform Resource Identifier commonly used in all HTTP(S) requests.
 */
struct URI {
    using QueryParams = util::CaseInsensitiveMap;

    /**
     * @brief Returns a URI object parsed from the given string.
     *
     * @param uri The string to parse.
     * @return URI The parsed URI.
     */
    [[nodiscard]] EKISOCKET_EXPORT static URI parse(const std::string& uri);

    /**
     * @brief Helper function to add a query parameter to the URI. Note: You will need to call the to_string() again to
     * see the changes.
     * @param key The key of the query parameter.
     * @param value The value of the query parameter.
     */
    EKISOCKET_EXPORT void add_query_parameter(const std::string& key, const std::string& value);

    /**
     * @brief Parses the given map of query parameters and adds them to the URI. Note: You will need to call the
     * to_string() again to see the changes.
     * @param params The map of query parameters.
     */
    EKISOCKET_EXPORT void add_query_parameters(const QueryParams& params);

    /**
     * @brief Converts the URI to a string.
     * @return  std::string The URI as a string.
     */
    [[nodiscard]] EKISOCKET_EXPORT std::string to_string() const;

    std::string scheme {};
    std::string username {};
    std::string password {};
    std::string host {};
    uint16_t port {};
    std::string path {};
    std::string query {};
    std::string fragment {};

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

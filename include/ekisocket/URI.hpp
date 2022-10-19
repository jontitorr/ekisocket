#pragma once
#include <ekisocket/Util.hpp>
#include <ekisocket_export.h>
#include <map>
#include <optional>
#include <string>

namespace ekisocket::http {
/**
 * @brief Represents a Uniform Resource Identifier commonly used in all HTTP(S) requests.
 */
struct Uri {
    using QueryParams = util::CaseInsensitiveMap;

    /**
     * @brief Returns a URI object parsed from the given string.
     *
     * @param uri The string to parse.
     * @return URI The parsed URI.
     */
    [[nodiscard]] EKISOCKET_EXPORT static Uri parse(std::string_view uri);

    std::string scheme {};
    std::string username {};
    std::string password {};
    std::string host {};
    std::optional<uint16_t> port {};
    std::string path {};
    QueryParams query {};
    std::string fragment {};
};
} // namespace ekisocket::http

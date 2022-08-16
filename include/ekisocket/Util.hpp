#pragma once
#include <cstdint>
#include <ekisocket_export.h>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace ekisocket::util {
/**
 * @brief Generates a random base64 encoded string from a theoretical source length.
 *
 * @param source_len The theoretical source length.
 * @return std::string The random base64 encoded string.
 */
std::string get_random_base64_from(uint32_t source_len);
/**
 * @brief Generates a random 32-bit unsigned integer within a range (defaults to [0, UINT32_MAX]).
 *
 * @param min The minimum value.
 * @param max The maximum value.
 * @return uint32_t A random number between in that range.
 */
uint32_t get_random_number(uint32_t min = 0, uint32_t max = (std::numeric_limits<uint32_t>::max)());
/**
 * @brief Encodes an input string of a certain length into a base64 encoded string.
 *
 * @param input The input string.
 * @param len The length of the input string.
 * @return std::string The base64 encoded string.
 */
[[nodiscard]] EKISOCKET_EXPORT std::string base64_encode(const uint8_t* input, size_t len);
[[nodiscard]] EKISOCKET_EXPORT std::string base64_encode(std::string_view input);
/**
 * @brief Computes the "Sec-WebSocket-Accept" header found in the handshake response to a WebSocket connection, given the "Sec-WebSocket-Key" sent to the server.
 *
 * @param key The "Sec-WebSocket-Key" sent to the server.
 * @return std::string The computed "Sec-WebSocket-Accept" header.
 */
std::string compute_accept(const std::string& key);
/**
 * @brief Encodes a 32-bit unsigned integer into a base64 encoded string.
 *
 * @param triple The 32-bit unsigned integer to encode.
 * @param num_of_bits The number of bits to encode.
 * @return std::string The base64 encoded string.
 */
std::string encode_triple(uint32_t triple, uint8_t num_of_bits);
/**
 * @brief Get the base64 char object for a certain index.
 *
 * @param c The index of the base64 char.
 * @return char The base64 char.
 */
char get_base64_char(uint8_t c);

/* ------------------ String Helper Function (not all used) ----------------- */

/**
 * @brief Performs a case insensitive string comparison.
 *
 * @param a The first string.
 * @param b The second string.
 * @return Whether they are the same string regardless of case.
 */
bool iequals(const std::string& a, const std::string& b);
std::string& ltrim(std::string& s);
std::string& rtrim(std::string& s);
std::string& trim(std::string& s);
std::vector<std::string> split(std::string_view s, std::string_view delimiter);
std::string join(const std::vector<std::string>& v, std::string_view delimiter);

template <typename Iterator>
std::string join(Iterator begin, Iterator end, const std::string& delimiter)
{
    std::stringstream ss {};
    for (Iterator it = begin; it != end; ++it) {
        if (it != begin) {
            ss << delimiter;
        }
        ss << *it;
    }
    return ss.str();
}

bool is_number(std::string_view s);

/* -------------------------- HTTP Helper Functions ------------------------- */

/**
 * @brief Create a random boundary string that complies with RFC 2046.
 * @note A boundary string cannot exceed 70 characters, and must consist only of printable ASCII characters.
 *
 * @return std::string The random boundary string.
 */
[[nodiscard]] EKISOCKET_EXPORT std::string create_boundary();

/// Helper function for creating multipart/form-data requests
[[nodiscard]] EKISOCKET_EXPORT std::string create_multipart_form_data(const std::string& key, std::string_view value, const std::string& boundary);

/// Helper function for creating multipart/form-data requests with multiple key/value pairs
[[nodiscard]] EKISOCKET_EXPORT std::string create_multipart_form_data(const std::vector<std::pair<std::string, std::string>>& key_value_pairs, const std::string& boundary);

[[nodiscard]] EKISOCKET_EXPORT std::string create_multipart_form_data_file(const std::string& name, std::string_view file_contents, const std::string& filename, const std::string& boundary);

/// Helper function for creating application/x-www-form-urlencoded requests
[[nodiscard]] EKISOCKET_EXPORT std::string create_application_x_www_form_urlencoded(const std::string& key, const std::string& value);
} // namespace ekisocket::util

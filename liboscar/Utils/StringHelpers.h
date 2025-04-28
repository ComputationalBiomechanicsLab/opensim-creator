#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace osc
{
    // returns true if `sv` contains `substr`
    bool contains(std::string_view sv, std::string_view substr);

    // returns true if `sv` contains `c`
    bool contains(std::string_view sv, std::string_view::value_type c);

    // returns true if `sv` contains `substr` (case-insensitive)
    bool contains_case_insensitive(std::string_view sv, std::string_view substr);

    // returns true if `b` is lexicographically greater than `a`, ignoring case
    bool is_string_case_insensitive_greater_than(std::string_view a, std::string_view b);

    // returns true if `a` is equal to `b` (case-insensitive)
    bool is_equal_case_insensitive(std::string_view a, std::string_view b);

    // returns true if:
    //
    // - `sv` is nonempty
    // - the first character of `sv` is in the set [_a-zA-Z]
    // - the remaining characters of `sv` are in the set [_a-zA-Z0-9]
    // - (sorry UTF8ers)
    //
    // i.e. it would be a valid identifier in, say, a scripting language or tree
    bool is_valid_identifier(std::string_view sv);

    // returns a substring of `sv` without leading/trailing whitespace
    std::string_view strip_whitespace(std::string_view sv);

    // (tries to) convert `sv` to a floating point number
    //
    // - strips leading and trailing whitespace
    //
    // - parses the remaining characters as a locale-dependent floating point
    //   number, internally using something like `std::strtof` (which depends
    //   on C locale - careful)
    //
    // returns the resulting float if successful, or `std::nullopt` if it fails
    std::optional<float> from_chars_strip_whitespace(std::string_view sv);

    // returns a string that *may* be truncated with ellipsis (...) if the length
    // of `sv` exceeds `max_length`
    std::string truncate_with_ellipsis(std::string_view sv, size_t max_length);

    // returns the end of the string between the last occurrence of `delimiter` and
    // the end of `sv`, or `sv` if `delimiter` does not occur within `sv`.
    std::string_view substring_after_last(std::string_view sv, std::string_view::value_type delimiter);

    // converts the given byte into a 2-length hex character representation
    //
    // e.g. 0x00 --> ('0', '0')
    //      0xf0 --> ('f', '0')
    //      0x02 --> ('0', '2')
    std::pair<char, char> to_hex_chars(uint8_t);
    std::optional<uint8_t> try_parse_hex_chars_as_byte(char, char);

    // satisfied if `T` can be written to a `std::ostream`
    template<typename T>
    concept OutputStreamable = requires (T v, std::ostream& o) {
        { o << v } -> std::same_as<std::ostream&>;
    };

    // returns a string representation of `v` by first streaming it to a `std::stringstream`
    template<OutputStreamable T>
    std::string stream_to_string(const T& v)
    {
        std::stringstream ss;
        ss << v;
        return std::move(ss).str();
    }

    // returns a string that contains a string-ified version of each element in `r` joined
    // with a given `delimiter`.
    template<std::ranges::input_range R>
    requires OutputStreamable<std::ranges::range_value_t<R>>
    std::string join(R&& r, std::string_view delimiter)
    {
        std::stringstream ss;
        std::string_view prefix_delimiter;
        for (auto&& el : r) {
            ss << prefix_delimiter;
            ss << el;
            prefix_delimiter = delimiter;
        }
        return std::move(ss).str();
    }

    // Returns a copy of `str`'s content, but with the first instance of `from` replaced
    // with `to` (if any).
    std::string replace(std::string_view str, std::string_view from, std::string_view to);
}

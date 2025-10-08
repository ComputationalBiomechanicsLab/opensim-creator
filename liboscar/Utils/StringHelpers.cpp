#include "StringHelpers.h"

#include <liboscar/Utils/Algorithms.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

using namespace osc;
namespace rgs = std::ranges;

namespace
{
    constexpr auto c_nibble_to_character_lut = std::to_array(
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    });
}

bool osc::contains_case_insensitive(std::string_view sv, std::string_view substr)
{
    const auto tolower = [](const auto c) { return std::tolower(c); };
    return std::ranges::search(sv, substr, std::ranges::equal_to{}, tolower, tolower).size() == substr.size();
}

bool osc::is_string_case_insensitive_greater_than(std::string_view a, std::string_view b)
{
    // this is a more verbose implementation of:
    //
    //     https://stackoverflow.com/questions/33379846/case-insensitive-sorting-of-an-array-of-strings

    auto [a_iter, b_iter] = rgs::mismatch(a, b, rgs::equal_to{}, [](auto c) { return std::tolower(c); });

    if (b_iter == b.end()) {
        return false;  // `b` is not greater than `a` (they are equal)
    }

    if (a_iter == a.end()) {
        return true;  // `b` is greater than `a` (common prefix, but has more letters)
    }

    // true if `b` is greater than `a` (first mismatching character is greater)
    // else, `a` is greater than `b` (first mismatching character is less)
    return std::tolower(*a_iter) < std::tolower(*b_iter);
}

bool osc::is_equal_case_insensitive(std::string_view a, std::string_view b)
{
    return rgs::equal(a, b, rgs::equal_to{}, [](auto c) { return std::tolower(c); });
}

bool osc::is_valid_identifier(std::string_view sv)
{
    // helpers
    const auto is_valid_first_character_of_identifier = [](std::string_view::value_type c)
    {
        return
            ('A' <= c && c <= 'Z') or
            (     '_' == c       ) or
            ('a' <= c && c <= 'z');
    };
    const auto is_valid_non_first_character_of_identifier = [](std::string_view::value_type c)
    {
        return
            ('0' <= c && c <= '9') or
            ('A' <= c && c <= 'Z') or
            (     '_' == c       ) or
            ('a' <= c && c <= 'z');
    };

    if (sv.empty()) {
        return false;
    }

    if (not is_valid_first_character_of_identifier(sv.front())) {
        return false;
    }

    return rgs::all_of(sv.begin() + 1, sv.end(), is_valid_non_first_character_of_identifier);
}

std::string_view osc::strip_whitespace(std::string_view sv)
{
    const auto front = rgs::find_if_not(sv, ::isspace);
    const auto back = rgs::find_if_not(sv.rbegin(), std::string_view::const_reverse_iterator{front}, ::isspace).base();
    return {sv.data() + std::distance(sv.begin(), front), static_cast<size_t>(std::distance(front, back))};
}

std::optional<float> osc::from_chars_strip_whitespace(std::string_view sv)
{
    sv = strip_whitespace(sv);

    if (sv.empty()) {
        return std::nullopt;
    }

    size_t i = 0;
    float fpv = 0.0f;

    try {
        fpv = std::stof(std::string{sv}, &i);
    }
    catch (const std::invalid_argument&) {
        // no conversion could be performed
        return std::nullopt;
    }
    catch (const std::out_of_range&) {
        // value is out of the range of representable values of a float
        return std::nullopt;
    }

    // else: it m

    return i == sv.size() ? std::optional<float>{fpv} : std::optional<float>{};
}

std::string osc::truncate_with_ellipsis(std::string_view v, size_t max_length)
{
    if (v.length() <= max_length) {
        return std::string{v};
    }

    const std::string_view substring = v.substr(0, max(0z, static_cast<ptrdiff_t>(max_length)-3));
    std::string rv;
    rv.reserve(substring.length() + 3);
    rv = substring;
    rv += "...";
    return rv;
}

std::string_view osc::substring_after_last(std::string_view sv, std::string_view::value_type delimiter)
{
    const size_t pos = sv.rfind(delimiter);

    if (pos == std::string_view::npos) {
        return sv;  // `sv` is empty or contains no delimiter
    }
    else if (pos == sv.size()-1) {
        return {};  // `delimiter` is the last in `sv`
    }
    else {
        return sv.substr(pos+1);
    }
}

std::pair<char, char> osc::to_hex_chars(uint8_t b)
{
    static_assert((std::numeric_limits<decltype(b)>::max() & 0xf) < c_nibble_to_character_lut.size());
    static_assert(((std::numeric_limits<decltype(b)>::max()>>1) & 0xf) < c_nibble_to_character_lut.size());

    const char msn = c_nibble_to_character_lut[(b>>4) & 0xf];
    const char lsn = c_nibble_to_character_lut[b & 0xf];
    return {msn, lsn};
}

std::optional<uint8_t> osc::try_parse_hex_chars_as_byte(char a, char b)
{
    // you might be wondering why we aren't using a library function, it's
    // because:
    //
    // - `std::stringstream` is a sledgehammer that will try its best to parse
    //   all sorts of `n`-length strings as hex strings, so users of this
    //   function would need to know the edge-cases
    //
    // - `std::strtol` is closer, in that it supports parsing base16 strings etc.
    //   but it _also_ handles things such as plus/minus signs, the `0x`, octal,
    //   etc.
    //
    // - `std::from_chars`, is the savior from `std::strtol`, but _also_ has special
    //   parsing behavior (e.g. the test suite for this function found that it
    //   is effectively a wrapper around `std::strol` in terms of behavior`
    //
    // and all this particular function needs to do is map strings like '00' to
    // 0x0, 'ff' to 255, etc.
    //
    // feel free to change this and re-run the test suite if you can dream up a
    // better approach though (I know this is ass)


    const auto try_convert_nibble_char_to_uint8 = [](char c) -> std::optional<uint8_t>
    {
        if ('0' <= c && c <= '9') {
            return static_cast<uint8_t>(c - '0');
        }
        else if ('a' <= c && c <= 'f') {
            return static_cast<uint8_t>(10 + (c - 'a'));
        }
        else if ('A' <= c && c <= 'F') {
            return static_cast<uint8_t>(10 + (c - 'A'));
        }
        else {
            return std::nullopt;
        }
    };

    const auto most_significant_nibble = try_convert_nibble_char_to_uint8(a);
    const auto least_significant_nibble = try_convert_nibble_char_to_uint8(b);

    if (most_significant_nibble and least_significant_nibble) {
        const auto v = static_cast<uint8_t>((*most_significant_nibble << 4) | *least_significant_nibble);
        return v;
    }
    else {
        return std::nullopt;
    }
}

std::string osc::replace(std::string_view str, std::string_view from, std::string_view to)
{
    if (const auto pos = str.find(from); pos != std::string_view::npos) {
        std::string rv;
        rv.reserve(str.size() + to.size() - from.size());
        rv.insert(0, str.substr(0, pos));
        rv.insert(rv.size(), to);
        rv.insert(rv.size(), str.substr(pos + from.size()));
        return rv;
    }
    else {
        return std::string{str};
    }
}

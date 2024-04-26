#include "StringHelpers.h"

#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/Utils/Algorithms.h>

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
    constexpr auto c_NibbleToCharacterLUT = std::to_array(
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    });

    std::string ToLower(std::string_view sv)
    {
        std::string cpy{sv};
        std::transform(cpy.begin(), cpy.end(), cpy.begin(), [](std::string::value_type c)
        {
            return static_cast<std::string::value_type>(std::tolower(c));
        });
        return cpy;
    }
}

bool osc::Contains(std::string_view sv, std::string_view substr)
{
    const auto it = std::search(
        sv.begin(),
        sv.end(),
        substr.begin(),
        substr.end()
    );
    return it != sv.end();
}

bool osc::Contains(std::string_view sv, std::string_view::value_type c)
{
    return cpp23::contains(sv, c);
}

bool osc::ContainsCaseInsensitive(std::string_view sv, std::string_view substr)
{
    if (substr.empty())
    {
        return true;
    }

    if (substr.size() > sv.size())
    {
        return false;
    }

    const std::string s = ToLower(sv);
    const std::string ss = ToLower(substr);

    return Contains(s, ss);
}

bool osc::IsStringCaseInsensitiveGreaterThan(std::string_view a, std::string_view b)
{
    // this is a more verbose implementation of:
    //
    //     https://stackoverflow.com/questions/33379846/case-insensitive-sorting-of-an-array-of-strings

    auto [itA, itB] = rgs::mismatch(a, b, [](auto c1, auto c2)
    {
        return std::tolower(c1) == std::tolower(c2);
    });

    if (itB == b.end())
    {
        return false;  // b is not greater than a (they are equal)
    }

    if (itA == a.end())
    {
        return true;  // b is greater than a (common prefix, but has more letters)
    }

    // true if b is greater than a (first mismatching character is greater)
    // else, a is greater than b (first mismatching character is less)
    return std::tolower(*itA) < std::tolower(*itB);
}

bool osc::is_equal_case_insensitive(std::string_view a, std::string_view b)
{
    const auto compareChars = [](std::string_view::value_type c1, std::string_view::value_type c2)
    {
        return std::tolower(static_cast<std::make_unsigned_t<decltype(c1)>>(c1)) == std::tolower(static_cast<std::make_unsigned_t<decltype(c2)>>(c2));
    };

    return rgs::equal(a, b, compareChars);
}

bool osc::is_valid_identifier(std::string_view sv)
{
    // helpers
    const auto isValidFirstCharacterOfIdentifier = [](std::string_view::value_type c)
    {
        return
            ('A' <= c && c <= 'Z') ||
            (     '_' == c       ) ||
            ('a' <= c && c <= 'z');
    };
    const auto isValidTrailingCharacterOfIdentifier = [](std::string_view::value_type c)
    {
        return
            ('0' <= c && c <= '9') ||
            ('A' <= c && c <= 'Z') ||
            (     '_' == c       ) ||
            ('a' <= c && c <= 'z');
    };

    if (sv.empty())
    {
        return false;
    }
    else if (!isValidFirstCharacterOfIdentifier(sv.front()))
    {
        return false;
    }
    else
    {
        return std::all_of(sv.begin() + 1, sv.end(), isValidTrailingCharacterOfIdentifier);
    }
}

std::string_view osc::TrimLeadingAndTrailingWhitespace(std::string_view sv)
{
    const std::string_view::const_iterator front = rgs::find_if_not(sv, ::isspace);
    const std::string_view::const_iterator back = rgs::find_if_not(sv.rbegin(), std::string_view::const_reverse_iterator{front}, ::isspace).base();
    return {sv.data() + std::distance(sv.begin(), front), static_cast<size_t>(std::distance(front, back))};
}

std::optional<float> osc::FromCharsStripWhitespace(std::string_view sv)
{
    sv = TrimLeadingAndTrailingWhitespace(sv);

    if (sv.empty())
    {
        return std::nullopt;
    }

    size_t i = 0;
    float fpv = 0.0f;

    try
    {
        fpv = std::stof(std::string{sv}, &i);
    }
    catch (const std::invalid_argument&)
    {
        // no conversion could be performed
        return std::nullopt;
    }
    catch (const std::out_of_range&)
    {
        // value is out of the range of representable values of a float
        return std::nullopt;
    }

    // else: it m

    return i == sv.size() ? std::optional<float>{fpv} : std::optional<float>{};
}

std::string osc::Ellipsis(std::string_view v, size_t maxLen)
{
    if (v.length() <= maxLen)
    {
        return std::string{v};
    }

    std::string_view substr = v.substr(0, max(static_cast<ptrdiff_t>(0), static_cast<ptrdiff_t>(maxLen)-3));
    std::string rv;
    rv.reserve(substr.length() + 3);
    rv = substr;
    rv += "...";
    return rv;
}

std::string_view osc::SubstringAfterLast(std::string_view sv, std::string_view::value_type delimiter)
{
    const size_t pos = sv.rfind(delimiter);

    if (pos == std::string_view::npos)
    {
        return sv;  // `sv` is empty or contains no delimiter
    }
    else if (pos == sv.size()-1)
    {
        return {};  // `delimiter` is the last in `sv`
    }
    else
    {
        return sv.substr(pos+1);
    }
}

std::pair<char, char> osc::to_hex_chars(uint8_t b)
{
    static_assert((std::numeric_limits<decltype(b)>::max() & 0xf) < c_NibbleToCharacterLUT.size());
    static_assert(((std::numeric_limits<decltype(b)>::max()>>1) & 0xf) < c_NibbleToCharacterLUT.size());

    const char msn = c_NibbleToCharacterLUT[(b>>4) & 0xf];
    const char lsn = c_NibbleToCharacterLUT[b & 0xf];
    return {msn, lsn};
}

std::optional<uint8_t> osc::try_parse_hex_chars_as_byte(char a, char b)
{
    // you might be wondering why we aren't using a library function, it's
    // because:
    //
    // - std::stringstream is a sledge-hammer that will try its best to parse
    //   alsorts of `n`-lengthed strings as hex strings, so users of this
    //   function would need to know the edge-cases
    //
    // - std::strtol is closer, in that it supports parsing base16 strings etc.
    //   but it _also_ handles things such as plus/minus signs, the `0x`, octal,
    //   etc.
    //
    // - std::from_chars, is the savior from std::strtol, but _also_ has special
    //   parsing behavior (e.g. the test suite for this function found that it
    //   is effectively a wrapper around std::strol in terms of behavior
    //
    // and all this particular function needs to do is map strings like '00' to
    // 0x0, 'ff' to 255, etc.
    //
    // feel free to change this and re-run the test suite if you can dream up a
    // better approach though (I know this is ass)


    const auto tryConvertToNibbleBinary = [](char c) -> std::optional<uint8_t>
    {
        if ('0' <= c && c <= '9')
        {
            return static_cast<uint8_t>(c - '0');
        }
        else if ('a' <= c && c <= 'f')
        {
            return static_cast<uint8_t>(10 + (c - 'a'));
        }
        else if ('A' <= c && c <= 'F')
        {
            return static_cast<uint8_t>(10 + (c - 'A'));
        }
        else
        {
            return std::nullopt;
        }
    };

    const auto msn = tryConvertToNibbleBinary(a);
    const auto lsn = tryConvertToNibbleBinary(b);

    if (msn && lsn) {
        const auto v = static_cast<uint8_t>((*msn << 4) | *lsn);
        return v;
    }
    else {
        return std::nullopt;
    }
}

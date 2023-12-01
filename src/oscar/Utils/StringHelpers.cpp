#include "StringHelpers.hpp"

#include <oscar/Utils/CStringView.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    auto constexpr c_NibbleToCharacterLUT = std::to_array(
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
    auto const it = std::search(
        sv.begin(),
        sv.end(),
        substr.begin(),
        substr.end()
    );
    return it != sv.end();
}

bool osc::Contains(std::string_view sv, std::string_view::value_type c)
{
    return std::find(sv.begin(), sv.end(), c) != sv.end();
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

    std::string const s = ToLower(sv);
    std::string const ss = ToLower(substr);

    return Contains(s, ss);
}

bool osc::IsStringCaseInsensitiveGreaterThan(std::string_view a, std::string_view b)
{
    // this is a more verbose implementation of:
    //
    //     https://stackoverflow.com/questions/33379846/case-insensitive-sorting-of-an-array-of-strings

    auto [itA, itB] = std::mismatch(a.begin(), a.end(), b.begin(), b.end(), [](auto c1, auto c2)
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

bool osc::IsEqualCaseInsensitive(std::string_view a, std::string_view b)
{
    auto const compareChars = [](std::string_view::value_type c1, std::string_view::value_type c2)
    {
        return std::tolower(static_cast<unsigned char>(c1)) == std::tolower(static_cast<unsigned char>(c2));
    };

    return std::equal(a.begin(), a.end(), b.begin(), b.end(), compareChars);
}

bool osc::IsValidIdentifier(std::string_view sv)
{
    // helpers
    auto const isValidFirstCharacterOfIdentifier = [](std::string_view::value_type c)
    {
        return
            ('A' <= c && c <= 'Z') ||
            (     '_' == c       ) ||
            ('a' <= c && c <= 'z');
    };
    auto const isValidTrailingCharacterOfIdentifier = [](std::string_view::value_type c)
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
    std::string_view::const_iterator const front = std::find_if_not(sv.begin(), sv.end(), ::isspace);
    std::string_view::const_iterator const back = std::find_if_not(sv.rbegin(), std::string_view::const_reverse_iterator{front}, ::isspace).base();
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
    catch (std::invalid_argument const&)
    {
        // no conversion could be performed
        return std::nullopt;
    }
    catch (std::out_of_range const&)
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

    std::string_view substr = v.substr(0, std::max(static_cast<ptrdiff_t>(0), static_cast<ptrdiff_t>(maxLen)-3));
    std::string rv;
    rv.reserve(substr.length() + 3);
    rv = substr;
    rv += "...";
    return rv;
}

std::string_view osc::SubstringAfterLast(std::string_view sv, std::string_view::value_type delimiter)
{
    size_t const pos = sv.rfind(delimiter);

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

std::pair<char, char> osc::ToHexChars(uint8_t b)
{
    static_assert((std::numeric_limits<decltype(b)>::max() & 0xf) < c_NibbleToCharacterLUT.size());
    static_assert(((std::numeric_limits<decltype(b)>::max()>>1) & 0xf) < c_NibbleToCharacterLUT.size());

    char const msn = c_NibbleToCharacterLUT[(b>>4) & 0xf];
    char const lsn = c_NibbleToCharacterLUT[b & 0xf];
    return {msn, lsn};
}

std::optional<uint8_t> osc::TryParseHexCharsAsByte(char a, char b)
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


    auto const tryConvertToNibbleBinary = [](char c) -> std::optional<uint8_t>
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

    auto const msn = tryConvertToNibbleBinary(a);
    auto const lsn = tryConvertToNibbleBinary(b);

    if (msn && lsn)
    {
        auto const v = static_cast<uint8_t>((*msn << 4) | *lsn);
        return v;
    }
    else
    {
        return std::nullopt;
    }
}

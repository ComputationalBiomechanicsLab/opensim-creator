#pragma once

#include "oscar/Utils/CStringView.hpp"

#include <cstddef>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace osc
{
    // returns true if `b` is lexographically greater than `a`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
    bool IsStringCaseInsensitiveGreaterThan(std::string const& a, std::string const& b);

    // returns true if `str` contains the supplied substring
    bool ContainsSubstring(std::string const& str, std::string_view substr);
    bool ContainsSubstring(std::string_view str, std::string_view substr);

    // returns a lower-cased version of a string
    std::string ToLower(std::string_view);

    // returns true if `str` is equivalent to `other` (case-insensitive)
    bool IsEqualCaseInsensitive(std::string const&, std::string const&);

    // returns true if `s` constains the supplied substring (case-insensitive)
    bool ContainsSubstringCaseInsensitive(std::string const& str, std::string const& substr);

    // returns true if `s` ends with `suffix`
    bool CStrEndsWith(CStringView, std::string_view suffix);

    // returns true if `str` contains `c`
    bool Contains(CStringView, char e);

    template<typename T>
    inline std::string StreamToString(T const& v)
    {
        std::stringstream ss;
        ss << v;
        return std::move(ss).str();
    }

    // returns true if `s` begins with `prefix`
    bool StartsWith(std::string_view s, std::string_view prefix);

    // returns a string view without its leading/trailing whitespace
    std::string_view TrimLeadingAndTrailingWhitespace(std::string_view);

    // (tries to) convert a char sequence into a floating point number
    //
    // - strips leading and trailing whitespace
    //
    // - parses the remaining characters as a locale-dependent floating point
    //   number, internally using something like std::strtof (which depends
    //   on C locale - careful)
    //
    // returns the resulting float if sucessful, or std::nullopt if it fails
    //
    // the reason this function exists is because, at time of writing, C++'s
    // <charconv> `std::from_chars` function isn't implemented in Mac OSX
    // or linux. When they are, feel free to nuke this from orbit.
    //
    // see the unittest suite for some of the more unusual things to consider
    std::optional<float> FromCharsStripWhitespace(std::string_view);

    // returns a string that *may* be truncated with ellipsis (...) if the length
    // of the input character sequence exceeds the given maximum length
    std::string Ellipsis(std::string_view, size_t maxLen);

    // returns the end of the string between the last occurance of the delimiter and
    // the end of the string, or the input string if the delimiter does not occur within
    // the string.
    std::string_view SubstringAfterLast(std::string_view, char delimiter);
}

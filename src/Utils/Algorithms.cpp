#include "Algorithms.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

int32_t osc::NumBitsSetIn(int32_t v)
{
    uint32_t uv = static_cast<uint32_t>(v);
    uint32_t i = 0;
    while (uv)
    {
        uv &= (uv - 1);
        ++i;
    }
    return static_cast<int32_t>(i);
}

int32_t osc::LeastSignificantBitIndex(int32_t v)
{
    uint32_t uv = static_cast<uint32_t>(v);
    uint32_t i = 0;
    while (!(uv & 0x1))
    {
        uv >>= 1;
        ++i;
    }
    return static_cast<int32_t>(i);
}

bool osc::IsStringCaseInsensitiveGreaterThan(std::string const& a, std::string const& b)
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

    if (tolower(*itA) < tolower(*itB))
    {
        return true;  // b is greater than a (first mismatching character is greater)
    }
    else
    {
        return false;  // a is greater than b (first mismatching character is less)
    }
}

bool osc::IsFilenameLexographicallyGreaterThan(std::filesystem::path const& p1, std::filesystem::path const& p2)
{
    return IsStringCaseInsensitiveGreaterThan(p1.filename().string(), p2.filename().string());
}

bool osc::IsSubpath(std::filesystem::path const& dir, std::filesystem::path const& pth)
{
    auto dirNumComponents = std::distance(dir.begin(), dir.end());
    auto pathNumComponents = std::distance(pth.begin(), pth.end());

    if (pathNumComponents < dirNumComponents)
    {
        return false;
    }

    return std::equal(dir.begin(), dir.end(), pth.begin());
}

bool osc::ContainsSubstring(std::string const& str, std::string_view substr)
{
    return str.find(substr) != std::string::npos;
}

std::string osc::ToLower(std::string const& s)
{
    std::string cpy = s;
    std::transform(cpy.begin(), cpy.end(), cpy.begin(), [](uint8_t c) { return std::tolower(c); });
    return cpy;
}

bool osc::IsEqualCaseInsensitive(std::string const& s1, std::string const& s2)
{
    auto compareChars = [](uint8_t c1, uint8_t c2)
    {
        return std::tolower(c1) == std::tolower(c2);
    };

    return std::equal(s1.begin(), s1.end(), s2.begin(), s2.end(), compareChars);
}

bool osc::ContainsSubstringCaseInsensitive(std::string const& str, std::string const& substr)
{
    if (substr.empty())
    {
        return true;
    }

    if (substr.size() > str.size())
    {
        return false;
    }

    std::string s = ToLower(str);
    std::string ss = ToLower(substr);

    return ContainsSubstring(s, ss);
}

bool osc::CStrEndsWith(char const* s, std::string_view suffix)
{
    size_t sLen = std::strlen(s);

    if (sLen < suffix.length())
    {
        return false;
    }

    char const* sEnd = s + sLen;

    return std::equal(sEnd - suffix.length(), sEnd, suffix.begin(), suffix.end());
}

bool osc::Contains(char const* s, char c)
{
    return std::strchr(s, c) != nullptr;
}

bool osc::StartsWith(std::string_view s, std::string_view prefix)
{
    return prefix.size() <= s.size() && std::equal(prefix.begin(), prefix.end(), s.begin());
}

std::string_view osc::TrimLeadingAndTrailingWhitespace(std::string_view v)
{
    std::string_view::const_iterator const front = std::find_if_not(v.begin(), v.end(), ::isspace);
    std::string_view::const_iterator const back = std::find_if_not(v.rbegin(), std::string_view::const_reverse_iterator{front}, ::isspace).base();
    return {v.data() + std::distance(v.begin(), front), static_cast<size_t>(std::distance(front, back))};
}

std::optional<float> osc::FromCharsStripWhitespace(std::string_view v)
{
    v = TrimLeadingAndTrailingWhitespace(v);

    if (v.empty())
    {
        return std::nullopt;
    }

    size_t i = 0;
    float fpv = 0.0f;

    try
    {
        fpv = std::stof(std::string{v}, &i);
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

    return i == v.size() ? std::optional<float>{fpv} : std::optional<float>{};
}

std::string osc::Ellipsis(std::string_view v, int32_t maxLen)
{
    if (v.length() <= maxLen)
    {
        return std::string{v};
    }

    std::string_view substr = v.substr(0, std::max(0, maxLen-3));
    std::string rv;
    rv.reserve(substr.length() + 3);
    rv = substr;
    rv += "...";
    return rv;
}
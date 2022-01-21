#include "Algorithms.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>

int osc::NumBitsSetIn(int v) noexcept
{
    unsigned uv = static_cast<unsigned>(v);
    unsigned i = 0;
    while (uv)
    {
        uv &= (uv - 1);
        ++i;
    }
    return static_cast<int>(i);
}

int osc::LeastSignificantBitIndex(int v) noexcept
{
    unsigned uv = static_cast<unsigned>(v);
    unsigned i = 0;
    while (!(uv & 0x1))
    {
        uv >>= 1;
        ++i;
    }
    return static_cast<int>(i);
}

bool osc::IsStringCaseInsensitiveGreaterThan(std::string const& a, std::string const& b) noexcept
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


// returns true if `b` is lexographically greater than `a`, ignoring case
//
// e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
bool osc::IsFilenameLexographicallyGreaterThan(std::filesystem::path const& p1, std::filesystem::path const& p2) noexcept
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

bool osc::ContainsSubstring(std::string const& str, std::string const& substr)
{
    return str.find(substr) != std::string::npos;
}

std::string osc::ToLower(std::string const& s)
{
    std::string cpy = s;
    std::transform(cpy.begin(), cpy.end(), cpy.begin(), [](unsigned char c) { return std::tolower(c); });
    return cpy;
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

bool osc::CStrEndsWith(char const* s, std::string_view suffix) noexcept
{
    size_t sLen = std::strlen(s);

    if (sLen < suffix.length())
    {
        return false;
    }

    char const* sEnd = s + sLen;

    return std::equal(sEnd - suffix.length(), sEnd, suffix.begin(), suffix.end());
}

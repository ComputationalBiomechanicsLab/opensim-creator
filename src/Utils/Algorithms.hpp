#pragma once

#include <algorithm>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace osc {

    // remove all elements `e` in `Container` `c` for which `p(e)` returns `true`
    template<typename Container, typename UnaryPredicate>
    void RemoveErase(Container& c, UnaryPredicate p) {
        using std::begin;
        using std::end;

        auto endIt = end(c);
        auto it = std::remove_if(begin(c), endIt, p);
        c.erase(it, endIt);
    }

    template<typename Container, typename UnaryPredicate>
    bool AnyOf(Container const& c, UnaryPredicate p) {
        using std::begin;
        using std::end;

        return std::any_of(begin(c), end(c), p);
    }

    template<typename Container, typename UnaryPredicate>
    bool AllOf(Container const& c, UnaryPredicate p)
    {
        using std::begin;
        using std::end;

        return std::all_of(begin(c), end(c), p);
    }

    template<typename Container, typename UnaryPredicate>
    auto FindIf(Container const& c, UnaryPredicate p) {
        using std::begin;
        using std::end;

        return std::find_if(begin(c), end(c), p);
    }

    template<typename T>
    bool Contains(std::unordered_set<T> const& set, T const& value)
    {
        return set.find(value) != set.end();
    }

    template<typename T>
    bool Contains(std::vector<T> const& vec, T const& val)
    {
        return std::find(vec.begin(), vec.end(), val) != vec.end();
    }

    template<typename T, typename UnaryPredicate>
    bool ContainsIf(std::vector<T> const& vec, UnaryPredicate p)
    {
        return FindIf(vec, p) != vec.end();
    }

    template<typename K, typename V, typename K2>
    bool ContainsKey(std::unordered_map<K, V> const& map, K2 const& key)
    {
        return map.find(key) != map.end();
    }

    template<typename Container, typename Compare>
    void Sort(Container& c, Compare comp)
    {
        using std::begin;
        using std::end;

        std::sort(begin(c), end(c), comp);
    }

    // returns the number of bits set in the input integer
    //
    // e.g. 0x1 --> 1, 0x2 --> 1, 0x3 --> 2, 0xf --> 4
    int NumBitsSetIn(int v) noexcept;

    // returns the bit-index of the least significant bit that it set
    //
    // e.g. 0x1 --> 0, 0x2 --> 1, 0x3 --> 0, 0x4 --> 2
    int LeastSignificantBitIndex(int v) noexcept;

    // returns true if `b` is lexographically greater than `a`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
    bool IsStringCaseInsensitiveGreaterThan(std::string const& a, std::string const& b) noexcept;

    // returns true if `b` is lexographically greater than `a`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
    bool IsFilenameLexographicallyGreaterThan(std::filesystem::path const& p1, std::filesystem::path const& p2) noexcept;

    // returns true if `path` is within `dir` (non-recursive)
    bool IsSubpath(std::filesystem::path const& dir, std::filesystem::path const& path);

    // returns true if `str` contains the supplied substring
    bool ContainsSubstring(std::string const& str, std::string const& substr);

    // returns a lower-cased version of a string
    std::string ToLower(std::string const&);

    // returns true if `s` constains the supplied substring (case-insensitive)
    bool ContainsSubstringCaseInsensitive(std::string const& str, std::string const& substr);

    // returns true if `s` ends with `suffix`
    bool CStrEndsWith(char const* s, std::string_view suffix) noexcept;
}

#pragma once

#include <algorithm>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace osc {

    // remove all elements `e` in `Container` `c` for which `p(e)` returns `true`
    template<typename Container, typename UnaryPredicate>
    void RemoveErase(Container& c, UnaryPredicate p)
    {
        using std::begin;
        using std::end;

        auto endIt = end(c);
        auto it = std::remove_if(begin(c), endIt, p);
        c.erase(it, endIt);
    }

    // remove all elements in the given unordered set for which `p(*el)` returns `true`
    template<typename Key, typename Hash, typename KeyEqual, typename Alloc, typename UnaryPredicate>
    void RemoveErase(std::unordered_set<Key, Hash, KeyEqual, Alloc>& c, UnaryPredicate p)
    {
        // see: https://en.cppreference.com/w/cpp/container/unordered_set/erase_if

        for (auto it = c.begin(), end = c.end(); it != end;)
        {
            if (p(*it))
            {
                it = c.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    template<typename Container, typename UnaryPredicate>
    bool AnyOf(Container const& c, UnaryPredicate p)
    {
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
    auto FindIf(Container const& c, UnaryPredicate p)
    {
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

    template<typename T>
    std::unordered_set<T> Intersect(std::unordered_set<T> const& a, std::unordered_set<T> const& b)
    {
        std::unordered_set<T> rv;
        for (T const& v : a)
        {
            if (Contains(b, v))
            {
                rv.insert(v);
            }
        }
        return rv;
    }

    // returns `true` if the values of `a` and `b` are effectively equal
    //
    // this algorithm is designed to be correct, rather than fast
    inline bool AreEffectivelyEqual(double a, double b) noexcept
    {
        // why:
        //
        //     http://realtimecollisiondetection.net/blog/?p=89
        //     https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison
        //
        // machine epsilon is only relevant for numbers < 1.0, so the epsilon
        // value must be scaled up to the magnitude of the operands if you need
        // a more-correct equality comparison

        double scaledEpsilon = std::max({1.0, a, b}) * std::numeric_limits<double>::epsilon();
        return std::abs(a - b) < scaledEpsilon;
    }

    template<typename T, typename U>
    inline bool Is(U const& v)
    {
        return typeid(v) == typeid(T);
    }

    template<typename T, typename U>
    inline bool DerivesFrom(U const& v)
    {
        return dynamic_cast<T const*>(&v);
    }

    // returns the number of bits set in the input integer
    //
    // e.g. 0x1 --> 1, 0x2 --> 1, 0x3 --> 2, 0xf --> 4
    int NumBitsSetIn(int v);

    // returns the bit-index of the least significant bit that it set
    //
    // e.g. 0x1 --> 0, 0x2 --> 1, 0x3 --> 0, 0x4 --> 2
    int LeastSignificantBitIndex(int v);

    // returns true if `b` is lexographically greater than `a`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
    bool IsStringCaseInsensitiveGreaterThan(std::string const& a, std::string const& b);

    // returns true if `b` is lexographically greater than `a`, ignoring case
    //
    // e.g. "b" > "a", "B" > "a" (this isn't true if case-sensitive)
    bool IsFilenameLexographicallyGreaterThan(std::filesystem::path const& p1, std::filesystem::path const& p2);

    // returns true if `path` is within `dir` (non-recursive)
    bool IsSubpath(std::filesystem::path const& dir, std::filesystem::path const& path);

    // returns true if `str` contains the supplied substring
    bool ContainsSubstring(std::string const& str, std::string const& substr);

    // returns a lower-cased version of a string
    std::string ToLower(std::string const&);

    // returns true if `s` constains the supplied substring (case-insensitive)
    bool ContainsSubstringCaseInsensitive(std::string const& str, std::string const& substr);

    // returns true if `s` ends with `suffix`
    bool CStrEndsWith(char const* s, std::string_view suffix);

    // returns true if `str` contains `c`
    bool Contains(char const* str, char e);

    // combines hash of `T` into the seed value
    template <class T>
    inline size_t DoHashCombine(size_t seed, T const& v)
    {
        std::hash<T> hasher;
        return seed ^ hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    template<typename T>
    inline size_t HashOf(T const& v)
    {
        return std::hash<T>{}(v);
    }

    template<typename T, typename... Ts>
    inline size_t HashOf(T const& v, Ts const&... vs)
    {
        return DoHashCombine(HashOf(v), HashOf(vs...));
    }
}

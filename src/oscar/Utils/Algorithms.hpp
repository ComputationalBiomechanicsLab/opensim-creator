#pragma once

#include "oscar/Utils/CStringView.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <future>
#include <iterator>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace osc
{
    // perform a parallelized and "Chunked" ForEach, where each thread receives an
    // independent chunk of data to process
    //
    // this is a poor-man's `std::execution::par_unseq`, because C++17's <execution>
    // isn't fully integrated into MacOS/Linux
    template<typename T, typename UnaryFunction>
    void ForEachParUnseq(size_t minChunkSize, nonstd::span<T> vals, UnaryFunction f)
    {
        size_t const chunkSize = std::max(minChunkSize, vals.size()/std::thread::hardware_concurrency());
        size_t const remainder = vals.size() % chunkSize;
        size_t const nTasks = vals.size()/chunkSize;

        if (nTasks > 1)
        {
            std::vector<std::future<void>> tasks;
            tasks.reserve(nTasks);

            for (size_t i = 0; i < nTasks-1; ++i)
            {
                size_t const chunkBegin = i * chunkSize;
                size_t const chunkEnd = (i+1) * chunkSize;
                tasks.push_back(std::async(std::launch::async, [vals, f, chunkBegin, chunkEnd]()
                {
                    for (size_t j = chunkBegin; j < chunkEnd; ++j)
                    {
                        f(vals[j]);
                    }
                }));
            }

            // last worker must also handle the remainder
            {
                size_t const chunkBegin = (nTasks-1) * chunkSize;
                size_t const chunkEnd = vals.size();
                tasks.push_back(std::async(std::launch::async, [vals, f, chunkBegin, chunkEnd]()
                {
                    for (size_t i = chunkBegin; i < chunkEnd; ++i)
                    {
                        f(vals[i]);
                    }
                }));
            }

            for (std::future<void>& task : tasks)
            {
                task.get();
            }
        }
        else
        {
            // chunks would be too small if parallelized: just do it sequentially
            for (T& val : vals)
            {
                f(val);
            }
        }
    }

    template<typename T, size_t N, typename... Initializers>
    constexpr auto MakeSizedArray(Initializers&&... args) -> std::array<T, sizeof...(args)>
    {
        static_assert(sizeof...(args) == N);
        return {T(std::forward<Initializers>(args))...};
    }

    template<typename T, typename... Initializers>
    constexpr auto MakeArray(Initializers&&... args) -> std::array<T, sizeof...(args)>
    {
        return {T(std::forward<Initializers>(args))...};
    }

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
        // see C++20: https://en.cppreference.com/w/cpp/container/unordered_set/erase_if

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

    template<typename Container>
    void TransferToEnd(Container& src, Container& dest)
    {
        dest.reserve(dest.size() + src.size());
        std::move(std::make_move_iterator(src.begin()),
                  std::make_move_iterator(src.end()),
                  std::back_insert_iterator(dest));
        src.clear();
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

    template<typename Container, typename T>
    auto Find(Container const& c, T const& v)
    {
        using std::begin;
        using std::end;

        return std::find(begin(c), end(c), v);
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
    inline bool IsEffectivelyEqual(double a, double b) noexcept
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

    inline bool IsLessThanOrEffectivelyEqual(double a, double b) noexcept
    {
        if (a <= b)
        {
            return true;
        }
        else
        {
            return IsEffectivelyEqual(a, b);
        }
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
    int32_t NumBitsSetIn(int32_t v);

    // returns the bit-index of the least significant bit that it set
    //
    // e.g. 0x1 --> 0, 0x2 --> 1, 0x3 --> 0, 0x4 --> 2
    int32_t LeastSignificantBitIndex(int32_t v);

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

    // combines hash of `T` into the seed value
    template<typename T>
    inline size_t HashCombine(size_t seed, T const& v)
    {
        std::hash<T> hasher;
        return seed ^ (hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2));
    }

    template<typename T>
    inline size_t HashOf(T const& v)
    {
        return std::hash<T>{}(v);
    }

    template<typename T, typename... Ts>
    inline size_t HashOf(T const& v, Ts const&... vs)
    {
        return HashCombine(HashOf(v), HashOf(vs...));
    }

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
    std::string Ellipsis(std::string_view, int32_t maxLen);

    // returns the index of the given variant type (impl)
    template<typename Variant, typename T, size_t I = 0>
    [[nodiscard]] constexpr size_t VariantIndexImpl() noexcept
    {
        if constexpr (I >= std::variant_size_v<Variant>)
        {
            return std::variant_size_v<Variant>;
        }
        else if constexpr (std::is_same_v<std::variant_alternative_t<I, Variant>, T>)
        {
            return I;
        }
        else
        {
            return VariantIndexImpl<Variant, T, I + 1>();
        }
    }

    // returns the index of the given variant type
    template<typename Variant, typename T>
    [[nodiscard]] constexpr size_t VariantIndex() noexcept
    {
        return VariantIndexImpl<Variant, T>();
    }

    template<typename... Ts>
    struct Overload : Ts... {
        using Ts::operator()...;
    };
    template<typename... Ts> Overload(Ts...) -> Overload<Ts...>;
}

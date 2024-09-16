#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <ranges>
#include <utility>

namespace osc
{
    template<typename T>
    concept Hashable = requires(T v) {
        { std::hash<T>{}(v) } -> std::convertible_to<size_t>;
    };

    // combines hash of `T` into the seed value
    template<Hashable T>
    size_t hash_combine(size_t seed, const T& v)
    {
        return seed ^ (std::hash<T>{}(v) + 0x9e3779b9 + (seed<<6) + (seed>>2));
    }

    template<Hashable T>
    size_t hash_of(const T& v)
    {
        return std::hash<T>{}(v);
    }

    template<Hashable T, Hashable... Ts>
    size_t hash_of(const T& v, const Ts&... vs)
    {
        return hash_combine(hash_of(v), hash_of(vs...));
    }

    template<typename Range>
    size_t hash_range(const Range& range)
    {
        size_t rv = 0;
        for (const auto& el : range) {
            rv = hash_combine(rv, el);
        }
        return rv;
    }

    // an osc-specific hashing object
    //
    // think of it as a `std::hash` that's used specifically in situations where
    // specializing `std::hash` might be a bad idea (e.g. on `std` library types
    // templated on other `std` library types, where there's a nonzero chance the
    template<typename Key>
    struct Hasher;

    template<typename T1, typename T2>
    struct Hasher<std::pair<T1, T2>> final {
        size_t operator()(const std::pair<T1, T2>& p) const
        {
            return hash_of(p.first, p.second);
        }
    };
}

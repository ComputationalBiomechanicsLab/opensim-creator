#pragma once

#include <cstddef>
#include <utility>

namespace osc
{
    // combines hash of `T` into the seed value
    template<typename T>
    size_t HashCombine(size_t seed, T const& v)
    {
        return seed ^ (std::hash<T>{}(v) + 0x9e3779b9 + (seed<<6) + (seed>>2));
    }

    template<typename T>
    size_t HashOf(T const& v)
    {
        return std::hash<T>{}(v);
    }

    template<typename T, typename... Ts>
    size_t HashOf(T const& v, Ts const&... vs)
    {
        return HashCombine(HashOf(v), HashOf(vs...));
    }
}

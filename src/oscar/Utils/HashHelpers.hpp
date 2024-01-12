#pragma once

#include <oscar/Utils/Concepts.hpp>

#include <cstddef>
#include <functional>

namespace osc
{
    // combines hash of `T` into the seed value
    template<Hashable T>
    size_t HashCombine(size_t seed, T const& v)
    {
        return seed ^ (std::hash<T>{}(v) + 0x9e3779b9 + (seed<<6) + (seed>>2));
    }

    template<Hashable T>
    size_t HashOf(T const& v)
    {
        return std::hash<T>{}(v);
    }

    template<Hashable T, Hashable... Ts>
    size_t HashOf(T const& v, Ts const&... vs)
    {
        return HashCombine(HashOf(v), HashOf(vs...));
    }
}

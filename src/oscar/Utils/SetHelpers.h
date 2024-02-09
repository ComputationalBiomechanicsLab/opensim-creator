#pragma once

#include <unordered_set>

namespace osc
{
    template<typename T>
    bool Contains(std::unordered_set<T> const& set, T const& value)
    {
        return set.find(value) != set.end();
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
}

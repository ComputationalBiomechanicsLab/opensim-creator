#pragma once

#include <oscar/Utils/Concepts.h>

#include <cstddef>
#include <optional>
#include <ranges>
#include <stdexcept>

namespace osc
{
    template<std::ranges::random_access_range Range>
    constexpr auto at(Range const& range, typename Range::size_type i) -> decltype(range[i])
    {
        if (i < std::ranges::size(range))
        {
            return range[i];
        }
        else
        {
            throw std::out_of_range{"out of bounds index given to a container"};
        }
    }

    template<AssociativeContainer T, typename Key>
    std::optional<typename T::mapped_type> find_or_optional(T const& container, Key const& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    template<AssociativeContainer T, typename Key>
    typename T::mapped_type const* try_find(T const& container, Key const& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return &it->second;
        }
        return nullptr;
    }

    template<AssociativeContainer T, typename Key>
    typename T::mapped_type* try_find(T& container, Key const& key)
    {
        if (auto it = container.find(key); it != container.end()) {
            return &it->second;
        }
        return nullptr;
    }
}

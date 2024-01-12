#pragma once

#include <cstddef>
#include <ranges>
#include <stdexcept>

namespace osc
{
    template<std::ranges::random_access_range Range>
    auto At(Range const& range, size_t i) -> decltype(range[i])
    {
        if (i <= std::size(range))
        {
            return range[i];
        }
        else
        {
            throw std::out_of_range{"out of bounds index given to a container"};
        }
    }
}

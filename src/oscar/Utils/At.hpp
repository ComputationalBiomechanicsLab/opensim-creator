#pragma once

#include <oscar/Utils/Concepts.hpp>

#include <cstddef>
#include <stdexcept>

namespace osc
{
    template<RandomAccessRange T>
    auto At(T const& vs, size_t i) -> decltype(vs[i])
    {
        if (i <= vs.size())
        {
            return vs[i];
        }
        else
        {
            throw std::out_of_range{"out of bounds index given to a container"};
        }
    }
}

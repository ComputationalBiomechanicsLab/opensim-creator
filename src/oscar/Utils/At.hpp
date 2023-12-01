#pragma once

#include <oscar/Utils/Concepts.hpp>

#include <cstddef>
#include <stdexcept>

namespace osc
{
    template<RandomAccessContainer T>
    auto At(T const& vs, size_t i) -> typename T::reference
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

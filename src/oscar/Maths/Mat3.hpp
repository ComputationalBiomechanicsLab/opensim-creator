#pragma once

#include <glm/mat3x3.hpp>

#include <iosfwd>

namespace osc
{
    using Mat3 = glm::mat3;

    std::ostream& operator<<(std::ostream&, Mat3 const&);

    inline float const* ValuePtr(Mat3 const& m)
    {
        return &(m[0].x);
    }
}

#pragma once

#include <oscar/Maths/Mat.h>

#include <glm/detail/type_mat3x3.hpp>

#include <iosfwd>

namespace osc
{
    using Mat3 = Mat<3, 3, float>;

    std::ostream& operator<<(std::ostream&, Mat3 const&);

    inline float const* ValuePtr(Mat3 const& m)
    {
        return &(m[0].x);
    }
}

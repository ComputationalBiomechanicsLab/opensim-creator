#pragma once

#include <oscar/Maths/Mat.hpp>

#include <glm/detail/type_mat4x3.hpp>

#include <iosfwd>

namespace osc
{
    using Mat4x3 = Mat<4, 3, float>;

    std::ostream& operator<<(std::ostream&, Mat4x3 const&);
}

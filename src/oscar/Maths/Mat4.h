#pragma once

#include <oscar/Maths/Mat.h>

#include <glm/detail/type_mat4x4.hpp>

#include <iosfwd>

namespace osc
{
    using Mat4 = Mat<4, 4, float>;

    std::ostream& operator<<(std::ostream&, Mat4 const&);

    inline float const* ValuePtr(Mat4 const& m)
    {
        return &(m[0].x);
    }

    inline float* ValuePtr(Mat4& m)
    {
        return &(m[0].x);
    }

    template<typename T>
    constexpr T Identity();

    template<>
    constexpr Mat4 Identity()
    {
        return Mat4{1.0f};
    }
}

#pragma once

#include <glm/mat4x4.hpp>

#include <iosfwd>

namespace osc
{
    using Mat4 = glm::mat4;

    std::ostream& operator<<(std::ostream&, Mat4 const&);

    constexpr float const* ValuePtr(Mat4 const& m)
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

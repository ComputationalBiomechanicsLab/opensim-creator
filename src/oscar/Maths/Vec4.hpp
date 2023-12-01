#pragma once

#include <oscar/Maths/Vec.hpp>

#include <glm/detail/type_vec4.hpp>

#include <iosfwd>

namespace osc
{
    using Vec4 = Vec<4, float>;

    std::ostream& operator<<(std::ostream&, Vec4 const&);

    inline float const* ValuePtr(Vec4 const& v)
    {
        return &(v.x);
    }
}

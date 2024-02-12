#pragma once

#include <oscar/Maths/Vec.h>

#include <glm/detail/type_vec2.hpp>

#include <cstdint>
#include <iosfwd>
#include <string>

namespace osc
{
    using Vec2 = Vec<2, float>;
    using Vec2i = Vec<2, int>;
    using Vec2u32 = Vec<2, uint32_t>;

    std::ostream& operator<<(std::ostream&, Vec2 const&);
    std::string to_string(Vec2 const&);

    inline float const* ValuePtr(Vec2 const& v)
    {
        return &(v.x);
    }
}

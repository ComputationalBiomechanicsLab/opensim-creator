#pragma once

#include <glm/vec2.hpp>

#include <cstdint>
#include <iosfwd>
#include <string>

namespace osc
{
    using Vec2 = glm::vec2;
    using Vec2i = glm::ivec2;
    using Vec2u32 = glm::vec<2, uint32_t>;

    std::ostream& operator<<(std::ostream&, Vec2 const&);
    std::string to_string(Vec2 const&);

    inline float const* ValuePtr(Vec2 const& v)
    {
        return &(v.x);
    }
}

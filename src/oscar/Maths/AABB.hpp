#pragma once

#include <glm/vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct AABB final {
        glm::vec3 min;
        glm::vec3 max;
    };

    bool operator==(AABB const&, AABB const&) noexcept;
    bool operator!=(AABB const&, AABB const&) noexcept;
    std::ostream& operator<<(std::ostream&, AABB const&);
}

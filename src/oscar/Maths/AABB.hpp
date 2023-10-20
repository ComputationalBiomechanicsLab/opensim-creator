#pragma once

#include <glm/vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct AABB final {
        glm::vec3 min;
        glm::vec3 max;

        friend bool operator==(AABB const&, AABB const&) noexcept;
        friend bool operator!=(AABB const&, AABB const&) noexcept;
        friend std::ostream& operator<<(std::ostream&, AABB const&);
    };
}

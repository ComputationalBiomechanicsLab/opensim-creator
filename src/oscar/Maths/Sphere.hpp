#pragma once

#include <glm/vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct Sphere final {
        glm::vec3 origin = {};
        float radius = 1.0f;
    };

    std::ostream& operator<<(std::ostream&, Sphere const&);
}

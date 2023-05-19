#pragma once

#include <glm/vec3.hpp>

#include <iosfwd>

namespace osc
{
    struct Line final {
        glm::vec3 origin;
        glm::vec3 dir;
    };

    std::ostream& operator<<(std::ostream&, Line const&);
}

#pragma once

#include <glm/mat4x3.hpp>

#include <iosfwd>

namespace osc
{
    using Mat4x3 = glm::mat4x3;

    std::ostream& operator<<(std::ostream&, Mat4x3 const&);
}

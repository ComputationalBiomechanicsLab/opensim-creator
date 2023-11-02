#pragma once

#include <glm/vec4.hpp>

#include <iosfwd>

namespace osc
{
    using Vec4 = glm::vec4;

    std::ostream& operator<<(std::ostream&, Vec4 const&);

    constexpr float const* ValuePtr(Vec4 const& v)
    {
        return &(v.x);
    }
}

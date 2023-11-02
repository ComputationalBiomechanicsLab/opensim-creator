#pragma once

#include <glm/vec3.hpp>

#include <iosfwd>

namespace osc
{
    using Vec3 = glm::vec3;
    using Vec3f = Vec3;
    using Vec3d = glm::dvec3;

    std::ostream& operator<<(std::ostream&, Vec3 const&);

    constexpr float const* ValuePtr(Vec3 const& v)
    {
        return &(v.x);
    }

    constexpr float* ValuePtr(Vec3& v)
    {
        return &(v.x);
    }
}

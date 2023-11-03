#pragma once

#include <oscar/Utils/HashHelpers.hpp>

#include <glm/vec3.hpp>

#include <cstddef>
#include <iosfwd>
#include <string>

namespace osc
{
    using Vec3 = glm::vec3;
    using Vec3f = Vec3;
    using Vec3d = glm::dvec3;

    std::ostream& operator<<(std::ostream&, Vec3 const&);
    std::string to_string(Vec3 const&);

    inline float const* ValuePtr(Vec3 const& v)
    {
        return &(v.x);
    }

    inline float* ValuePtr(Vec3& v)
    {
        return &(v.x);
    }
}

template<>
struct std::hash<osc::Vec3> final {
    size_t operator()(osc::Vec3 const& v) const
    {
        return osc::HashOf(v.x, v.y, v.z);
    }
};

#pragma once

#include <glm/gtx/quaternion.hpp>

#include <iosfwd>

namespace osc
{
    using Quat = glm::quat;

    std::ostream& operator<<(std::ostream&, Quat const&);

    template<typename T>
    constexpr T Identity();

    template<>
    constexpr Quat Identity()
    {
        return Quat{1.0f, 0.0f, 0.0f, 0.0f};
    }
}

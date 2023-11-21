#pragma once

#include <oscar/Maths/Qualifier.hpp>

#include <glm/gtx/quaternion.hpp>
#include <glm/detail/qualifier.hpp>

namespace osc
{
    template<typename T, Qualifier Q = glm::defaultp>
    using Qua = glm::qua<T, Q>;
}

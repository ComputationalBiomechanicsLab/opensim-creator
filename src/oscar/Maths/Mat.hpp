#pragma once

#include <oscar/Maths/LengthType.hpp>

#include <glm/detail/qualifier.hpp>

namespace osc
{
    template<LengthType C, LengthType R, typename T, glm::qualifier Q = glm::defaultp>
    using Mat = glm::mat<C, R, T, Q>;
}

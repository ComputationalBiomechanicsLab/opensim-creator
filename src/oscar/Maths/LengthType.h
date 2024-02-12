#pragma once

#include <glm/detail/qualifier.hpp>

namespace osc
{
    // alias, so that downstream templated code can just use `length_t` as the
    // template value type, rather than directly refering to glm
    using LengthType = glm::length_t;
}

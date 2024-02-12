#pragma once

#include <oscar/Maths/LengthType.h>
#include <oscar/Maths/Qualifier.h>

#include <glm/detail/qualifier.hpp>

namespace osc
{
    // define osc's `Vec` as an alias to glm
    //
    // this is so that:
    //
    // - there's one central place where we can change packing (here)
    // - downstream maths algorithms can be templated in terms of `Vec`
    // - downstream code only uses osc classes - so that it's more flexible to later library changes etc.
    template<LengthType L, typename T, Qualifier Q = glm::defaultp>
    using Vec = glm::vec<L, T, Q>;
}

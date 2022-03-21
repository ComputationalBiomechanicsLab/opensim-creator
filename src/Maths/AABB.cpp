#include "AABB.hpp"

#include "src/Bindings/GlmHelpers.hpp"

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, AABB const& aabb)
{
    return o << "AABB(min = " << aabb.min << ", max = " << aabb.max << ')';
}

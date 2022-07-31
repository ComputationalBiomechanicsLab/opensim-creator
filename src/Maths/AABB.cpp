#include "AABB.hpp"

#include "src/Bindings/GlmHelpers.hpp"

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, AABB const& aabb)
{
    return o << "AABB(min = " << aabb.min << ", max = " << aabb.max << ')';
}

bool osc::operator==(AABB const& a, AABB const& b) noexcept
{
    return a.min == b.min && a.max == b.max;
}

bool osc::operator!=(AABB const& a, AABB const& b) noexcept
{
    return !(a == b);
}
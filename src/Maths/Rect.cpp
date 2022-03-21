#include "Rect.hpp"

#include "src/Bindings/GlmHelpers.hpp"

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, Rect const& r)
{
    return o << "Rect(p1 = " << r.p1 << ", p2 = " << r.p2 << ")";
}

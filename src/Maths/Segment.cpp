#include "Segment.hpp"

#include "src/Bindings/GlmHelpers.hpp"

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, Segment const& d)
{
    return o << "Segment(p1 = " << d.p1 << ", p2 = " << d.p2 << ')';
}

#include "Sphere.hpp"

#include "src/Bindings/GlmHelpers.hpp"

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, Sphere const& s)
{
    return o << "Sphere(origin = " << s.origin << ", radius = " << s.radius << ')';
}

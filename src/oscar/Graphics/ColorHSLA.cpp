#include "ColorHSLA.hpp"

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, ColorHSLA const& c)
{
    return o << "ColorHSLA(h = " << c.h << ", s = " << c.s << ", l = " << c.l << ", a = " << c.a << ')';
}

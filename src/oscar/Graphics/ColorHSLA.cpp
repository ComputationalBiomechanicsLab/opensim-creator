#include "ColorHSLA.h"

#include <ostream>

std::ostream& osc::operator<<(std::ostream& o, const ColorHSLA& c)
{
    return o << "ColorHSLA(h = " << c.h << ", s = " << c.s << ", l = " << c.l << ", a = " << c.a << ')';
}

#include "ColorHSLA.h"

#include <ostream>

std::ostream& osc::operator<<(std::ostream& color, const ColorHSLA& c)
{
    return color << "ColorHSLA(h = " << c.h << ", s = " << c.s << ", l = " << c.l << ", a = " << c.a << ')';
}

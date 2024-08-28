#include "ColorHSLA.h"

#include <ostream>

std::ostream& osc::operator<<(std::ostream& color, const ColorHSLA& c)
{
    return color << "ColorHSLA(h = " << c.hue << ", s = " << c.saturation << ", l = " << c.lightness << ", a = " << c.alpha << ')';
}

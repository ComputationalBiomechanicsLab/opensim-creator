#include "LandmarkPair3D.h"

#include <iostream>

using osc::operator<<;

std::ostream& osc::operator<<(std::ostream& o, LandmarkPair3D const& p)
{
    return o << "LandmarkPair3D{Src = " << p.source << ", dest = " << p.destination << '}';
}

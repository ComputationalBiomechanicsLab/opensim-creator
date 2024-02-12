#include "LandmarkPair3D.h"

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, LandmarkPair3D const& p)
{
    return o << "LandmarkPair3D{Src = " << p.source << ", dest = " << p.destination << '}';
}

#include "LandmarkPair3D.h"

#include <oscar/Maths/VecFunctions.h>

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, LandmarkPair3D const& p)
{
    return o << "LandmarkPair3D{source = " << p.source << ", dest = " << p.destination << '}';
}

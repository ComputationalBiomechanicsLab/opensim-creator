#include "LandmarkPair3D.h"

#include <liboscar/Maths/VecFunctions.h>

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, const LandmarkPair3D& p)
{
    return o << "LandmarkPair3D{source = " << p.source << ", dest = " << p.destination << '}';
}

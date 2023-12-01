#include "LandmarkPair3D.hpp"

#include <iostream>

std::ostream& osc::operator<<(std::ostream& o, LandmarkPair3D const& p)
{
    using osc::operator<<;
    return o << "LandmarkPair3D{Src = " << p.source << ", dest = " << p.destination << '}';
}

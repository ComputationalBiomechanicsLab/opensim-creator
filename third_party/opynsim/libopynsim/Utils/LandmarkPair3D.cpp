#include "LandmarkPair3D.h"

#include <iostream>

using namespace opyn;

namespace
{
    template<typename T>
    std::ostream& write_human_readable(std::ostream& o, const LandmarkPair3D<T>& p)
    {
        return o << "LandmarkPair3D{source = " << p.source << ", dest = " << p.destination << '}';
    }
}

std::ostream& opyn::operator<<(std::ostream& o, const LandmarkPair3D<float>& p) { return write_human_readable(o, p); }
std::ostream& opyn::operator<<(std::ostream& o, const LandmarkPair3D<double>& p) { return write_human_readable(o, p); }

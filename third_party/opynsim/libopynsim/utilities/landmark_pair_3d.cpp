#include "landmark_pair_3d.h"

#include <iostream>

using namespace opyn;

namespace
{
    template<typename T>
    std::ostream& write_human_readable(std::ostream& o, const landmark_pair_3d<T>& p)
    {
        return o << "LandmarkPair3D{source = " << p.source << ", dest = " << p.destination << '}';
    }
}

std::ostream& opyn::operator<<(std::ostream& o, const landmark_pair_3d<float>& p) { return write_human_readable(o, p); }
std::ostream& opyn::operator<<(std::ostream& o, const landmark_pair_3d<double>& p) { return write_human_readable(o, p); }

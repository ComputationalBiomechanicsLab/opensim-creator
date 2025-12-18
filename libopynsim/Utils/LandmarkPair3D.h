#pragma once

#include <SimTKcommon/SmallMatrix.h>

#include <concepts>
#include <iosfwd>

namespace opyn
{
    // a single source-to-destination landmark pair in 3D space
    //
    // this is typically what the user/caller defines
    template<std::floating_point T>
    struct LandmarkPair3D {
        friend bool operator==(const LandmarkPair3D&, const LandmarkPair3D&) = default;

        SimTK::Vec<3, T> source{T{0.0}};
        SimTK::Vec<3, T> destination{T{0.0}};
    };

    std::ostream& operator<<(std::ostream&, const LandmarkPair3D<float>&);
    std::ostream& operator<<(std::ostream&, const LandmarkPair3D<double>&);
}

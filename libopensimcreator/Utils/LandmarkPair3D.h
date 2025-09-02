#pragma once

#include <liboscar/Maths/Vector3.h>

#include <concepts>
#include <iosfwd>

namespace osc
{
    // a single source-to-destination landmark pair in 3D space
    //
    // this is typically what the user/caller defines
    template<std::floating_point T>
    struct LandmarkPair3D {
        friend bool operator==(const LandmarkPair3D&, const LandmarkPair3D&) = default;

        Vector<T, 3> source{};
        Vector<T, 3> destination{};
    };

    std::ostream& operator<<(std::ostream&, const LandmarkPair3D<float>&);
    std::ostream& operator<<(std::ostream&, const LandmarkPair3D<double>&);
}

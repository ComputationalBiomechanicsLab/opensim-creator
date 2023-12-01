#pragma once

#include <oscar/Maths/Vec3.hpp>

#include <iosfwd>

namespace osc
{
    // a single source-to-destination landmark pair in 3D space
    //
    // this is typically what the user/caller defines
    struct LandmarkPair3D {
        friend bool operator==(LandmarkPair3D const&, LandmarkPair3D const&) = default;

        Vec3 source{};
        Vec3 destination{};
    };

    std::ostream& operator<<(std::ostream&, LandmarkPair3D const&);
}

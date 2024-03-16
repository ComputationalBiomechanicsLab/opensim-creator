#pragma once

#include <oscar/Maths/Vec3.h>

#include <iosfwd>

namespace osc
{
    // a plane expressed in general form (ax + by + cz + d = 0)
    //
    // - useful for maths calculations
    //
    // - see `Plane` for a struct that's generally easier to handle in
    //   practical rendering contexts
    struct AnalyticPlane final {

        friend constexpr bool operator==(AnalyticPlane const&, AnalyticPlane const&) = default;

        float distance = 0.0f;
        Vec3 normal = {0.0f, 1.0f, 0.0f};
    };

    std::ostream& operator<<(std::ostream&, AnalyticPlane const&);
}

#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Angle.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <span>
#include <vector>

namespace osc
{
    struct LatheGeometryParams final {
        friend bool operator==(const LatheGeometryParams&, const LatheGeometryParams&) = default;

        std::vector<Vec2> points = {{0.0f, -0.5f}, {0.5f, 0.0f}, {0.0f, 0.5f}};
        size_t num_segments = 12;
        Radians phi_start = Degrees{0};
        Radians phi_length = Degrees{360};
    };

    // returns a mesh with axial symmetry like vases. The lathe rotates around the Y axis.
    //
    // (ported from three.js:LatheGeometry)
    class LatheGeometry final : public Mesh {
    public:
        using Params = LatheGeometryParams;

        static constexpr CStringView name() { return "Lathe"; }

        explicit LatheGeometry(const Params& = {});
    };
}

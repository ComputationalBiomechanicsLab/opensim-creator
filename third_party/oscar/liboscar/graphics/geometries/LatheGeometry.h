#pragma once

#include <liboscar/graphics/Mesh.h>
#include <liboscar/maths/Angle.h>
#include <liboscar/maths/Vector2.h>
#include <liboscar/utils/CStringView.h>

#include <cstddef>
#include <vector>

namespace osc
{
    struct LatheGeometryParams final {
        friend bool operator==(const LatheGeometryParams&, const LatheGeometryParams&) = default;

        std::vector<Vector2> points = {{0.0f, -0.5f}, {0.5f, 0.0f}, {0.0f, 0.5f}};
        size_t num_segments = 12;
        Radians phi_start = Degrees{0};
        Radians phi_length = Degrees{360};
    };

    // returns a mesh with axial symmetry like vases. The lathe rotates around the Y axis.
    //
    // (ported from three.js:LatheGeometry)
    class LatheGeometry final {
    public:
        using Params = LatheGeometryParams;

        static constexpr CStringView name() { return "Lathe"; }

        explicit LatheGeometry(const Params& = {});

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}

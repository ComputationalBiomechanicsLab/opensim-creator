#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Angle.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>
#include <utility>

namespace osc
{
    struct CircleGeometryParams final {
        friend bool operator==(const CircleGeometryParams&, const CircleGeometryParams&) = default;

        float radius = 1.0f;
        size_t num_segments = 32;
        Radians theta_start = Degrees{0};
        Radians theta_length = Degrees{360};
    };

    class CircleGeometry final {
    public:
        using Params = CircleGeometryParams;

        static constexpr CStringView name() { return "Circle"; }

        explicit CircleGeometry(const Params& = Params{});

        const Mesh& mesh() const & { return mesh_; }
        Mesh&& mesh() && { return std::move(mesh_); }
        operator const Mesh& () const & { return mesh_; }
        operator Mesh () && { return std::move(mesh_); }
    private:
        Mesh mesh_;
    };
}

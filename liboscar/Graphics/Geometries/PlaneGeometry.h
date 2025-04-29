#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct PlaneGeometryParams final {
        friend bool operator==(const PlaneGeometryParams&, const PlaneGeometryParams&) = default;

        float width = 1.0f;
        float height = 1.0f;
        size_t num_width_segments = 1;
        size_t num_height_segments = 1;
    };

    class PlaneGeometry final : public Mesh {
    public:
        using Params = PlaneGeometryParams;

        static constexpr CStringView name() { return "Plane"; }

        explicit PlaneGeometry(const Params& = {});

        Vec3 normal() const { return {0.0f, 0.0f, 1.0f}; }
    };
}

#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Vector2.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct PlaneGeometryParams final {
        friend bool operator==(const PlaneGeometryParams&, const PlaneGeometryParams&) = default;

        Vector2 dimensions = {1.0f, 1.0f};
        Vector2uz num_segments = {1, 1};
    };

    class PlaneGeometry final : public Mesh {
    public:
        using Params = PlaneGeometryParams;

        static constexpr CStringView name() { return "Plane"; }

        explicit PlaneGeometry(const Params& = {});

        Vector3 normal() const { return {0.0f, 0.0f, 1.0f}; }
    };
}

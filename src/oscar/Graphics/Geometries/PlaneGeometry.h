#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

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
    };
}

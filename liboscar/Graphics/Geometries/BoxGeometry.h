#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct BoxGeometryParams final {
        friend bool operator==(const BoxGeometryParams&, const BoxGeometryParams&) = default;

        float width = 1.0f;
        float height = 1.0f;
        float depth = 1.0f;
        size_t num_width_segments = 1;
        size_t num_height_segments = 1;
        size_t num_depth_segments = 1;
    };

    class BoxGeometry : public Mesh {
    public:
        using Params = BoxGeometryParams;

        static constexpr CStringView name() { return "Box"; }

        explicit BoxGeometry(const Params& = Params{});
    };
}

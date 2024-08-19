#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    class BoxGeometry : public Mesh {
    public:
        static constexpr CStringView name() { return "Box"; }

        struct Params final {
            float width = 1.0f;
            float height = 1.0f;
            float depth = 1.0f;
            size_t num_width_segments = 1;
            size_t num_height_segments = 1;
            size_t num_depth_segments = 1;
        };

        explicit BoxGeometry(const Params& = Params{});
    };
}

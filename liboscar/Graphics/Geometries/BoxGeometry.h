#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct BoxGeometryParams final {
        friend bool operator==(const BoxGeometryParams&, const BoxGeometryParams&) = default;

        Vector3 dimensions = {1.0f, 1.0f, 1.0f};
        Vector3uz num_segments = {1, 1, 1};
    };

    class BoxGeometry : public Mesh {
    public:
        using Params = BoxGeometryParams;

        static constexpr CStringView name() { return "Box"; }

        explicit BoxGeometry(const Params& = Params{});
    };
}

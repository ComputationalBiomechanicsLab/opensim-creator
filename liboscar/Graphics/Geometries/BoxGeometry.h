#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>
#include <utility>

namespace osc
{
    struct BoxGeometryParams final {
        friend bool operator==(const BoxGeometryParams&, const BoxGeometryParams&) = default;

        Vector3 dimensions = {1.0f, 1.0f, 1.0f};
        Vector3uz num_segments = {1, 1, 1};
    };

    class BoxGeometry final {
    public:
        using Params = BoxGeometryParams;

        static constexpr CStringView name() { return "Box"; }

        explicit BoxGeometry(const Params& = Params{});

        const Mesh& mesh() const & { return mesh_; }
        Mesh&& mesh() && { return std::move(mesh_); }
        operator const Mesh& () const & { return mesh_; }
        operator Mesh () && { return std::move(mesh_); }
    private:
        Mesh mesh_;
    };
}

#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>
#include <utility>

namespace osc
{
    struct IcosahedronGeometryParams final {
        friend bool operator==(const IcosahedronGeometryParams&, const IcosahedronGeometryParams&) = default;

        float radius = 1.0f;
        size_t detail = 0;
    };

    class IcosahedronGeometry final {
    public:
        using Params = IcosahedronGeometryParams;

        static constexpr CStringView name() { return "Icosahedron"; }

        explicit IcosahedronGeometry(const Params& = {});

        const Mesh& mesh() const & { return mesh_; }
        Mesh&& mesh() && { return std::move(mesh_); }
        operator const Mesh& () const & { return mesh_; }
        operator Mesh () && { return std::move(mesh_); }
    private:
        Mesh mesh_;
    };
}

#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>
#include <utility>

namespace osc
{
    struct TetrahedronGeometryParams final {
        friend bool operator==(const TetrahedronGeometryParams&, const TetrahedronGeometryParams&) = default;

        float radius = 1.0f;
        size_t detail_level = 0;
    };

    class TetrahedronGeometry final {
    public:
        using Params = TetrahedronGeometryParams;

        static constexpr CStringView name() { return "Tetrahedron"; }

        explicit TetrahedronGeometry(const Params& = {});

        const Mesh& mesh() const & { return mesh_; }
        Mesh&& mesh() && { return std::move(mesh_); }
        operator const Mesh& () const & { return mesh_; }
        operator Mesh () && { return std::move(mesh_); }
    private:
        Mesh mesh_;
    };
}

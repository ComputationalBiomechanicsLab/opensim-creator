#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Maths/Vector3.h>
#include <liboscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    struct GridGeometryParams final {
        friend bool operator==(const GridGeometryParams&, const GridGeometryParams&) = default;

        float size = 2.0f;
        size_t num_divisions = 10;
    };

    class GridGeometry final {
    public:
        using Params = GridGeometryParams;

        static constexpr CStringView name() { return "Grid"; }

        explicit GridGeometry(const Params& = {});

        Vector3 normal() const { return {0.0f, 0.0f, 1.0f}; }

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}

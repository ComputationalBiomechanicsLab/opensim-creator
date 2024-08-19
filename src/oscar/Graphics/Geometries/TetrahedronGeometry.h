#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>

namespace osc
{
    class TetrahedronGeometry final : public Mesh {
    public:
        static constexpr CStringView name() { return "Tetrahedron"; }

        struct Params final {
            float radius = 1.0f;
            size_t detail_level = 0;
        };

        explicit TetrahedronGeometry(const Params& = {});
    };
}

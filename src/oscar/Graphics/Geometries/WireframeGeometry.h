#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Utils/CStringView.h>

namespace osc
{
    class WireframeGeometry final {
    public:
        static constexpr CStringView name() { return "Wireframe"; }

        explicit WireframeGeometry(const Mesh&);

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}

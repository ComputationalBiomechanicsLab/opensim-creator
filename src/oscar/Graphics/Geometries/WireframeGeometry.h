#pragma once

#include <oscar/Graphics/Mesh.h>

namespace osc
{
    class WireframeGeometry final {
    public:
        explicit WireframeGeometry(const Mesh&);

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}

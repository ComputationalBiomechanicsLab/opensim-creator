#pragma once

#include <liboscar/graphics/mesh.h>
#include <liboscar/utilities/c_string_view.h>

namespace osc
{
    class WireframeGeometry final {
    public:
        static constexpr CStringView name() { return "Wireframe"; }

        // default-constructs a `WireframeGeometry` of a default-constructed `BoxGeometry`
        explicit WireframeGeometry();

        explicit WireframeGeometry(const Mesh&);

        const Mesh& mesh() const { return mesh_; }
        operator const Mesh& () const { return mesh_; }
    private:
        Mesh mesh_;
    };
}

#pragma once

#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Utils/CStringView.h>

namespace osc
{
    class WireframeGeometry final : public Mesh {
    public:
        static constexpr CStringView name() { return "Wireframe"; }

        // default-constructs a `WireframeGeometry` of a default-constructed `BoxGeometry`
        explicit WireframeGeometry();

        explicit WireframeGeometry(const Mesh&);
    };
}

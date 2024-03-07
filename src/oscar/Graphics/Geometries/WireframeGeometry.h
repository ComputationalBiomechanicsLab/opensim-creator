#pragma once

#include <oscar/Graphics/Mesh.h>

namespace osc
{
    class WireframeGeometry final {
    public:
        explicit WireframeGeometry(Mesh const&);

        Mesh const& mesh() const { return m_Mesh; }
        operator Mesh const& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}

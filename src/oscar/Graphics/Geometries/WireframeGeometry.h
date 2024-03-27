#pragma once

#include <oscar/Graphics/Mesh.h>

namespace osc
{
    class WireframeGeometry final {
    public:
        explicit WireframeGeometry(const Mesh&);

        const Mesh& mesh() const { return m_Mesh; }
        operator const Mesh& () const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}

#pragma once

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/Vec3.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace osc
{
    // generates a 3D solid with flat faces by projecting triangle faces (`indicies`
    // indexes into `vertices` for each triangle) onto a sphere, followed by dividing
    // them up to the desired level of detail
    //
    // inspired by three.js's `PolyhedronGeometry`
    class PolyhedronGeometry final {
    public:
        PolyhedronGeometry(
            std::span<Vec3 const> vertices,
            std::span<uint32_t const> indices,
            float radius,
            size_t detail
        );

        operator Mesh const& () const { return m_Mesh; }
        Mesh const& mesh() const { return m_Mesh; }
    private:
        Mesh m_Mesh;
    };
}

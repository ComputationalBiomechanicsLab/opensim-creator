#pragma once

#include <OpenSim/Simulation/Model/Geometry.h>
#include <oscar/Graphics/MeshIndicesView.h>
#include <oscar/Maths/Vec3.h>
#include <Simbody.h>

#include <cstdint>
#include <span>

namespace osc
{
    // custom component for storing an in-memory mesh
    //
    // used for (e.g.) storing+showing warp results without having to persist
    // a mesh file to disk
    class InMemoryMesh : public OpenSim::Geometry {
        OpenSim_DECLARE_CONCRETE_OBJECT(InMemoryMesh, OpenSim::Geometry);
    public:
        InMemoryMesh() = default;
        InMemoryMesh(std::span<Vec3 const>, MeshIndicesView);

        void implementCreateDecorativeGeometry(SimTK::Array_<SimTK::DecorativeGeometry>&) const override;
    private:
        SimTK::PolygonalMesh m_MeshData;
    };
}

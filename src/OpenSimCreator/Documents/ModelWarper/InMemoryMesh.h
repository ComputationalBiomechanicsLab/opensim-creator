#pragma once

#include <OpenSim/Simulation/Model/Geometry.h>
#include <Simbody.h>

namespace osc
{
    // custom component for storing an in-memory mesh
    //
    // used for (e.g.) storing+showing warp results without having to persist
    // a mesh file to disk
    class InMemoryMesh : public OpenSim::Geometry {
        OpenSim_DECLARE_CONCRETE_OBJECT(InMemoryMesh, OpenSim::Geometry);
    public:
        void implementCreateDecorativeGeometry(SimTK::Array_<SimTK::DecorativeGeometry>& out) const override
        {
            out.push_back(SimTK::DecorativeMesh{m_MeshData});
        }
    private:
        SimTK::PolygonalMesh m_MeshData;
    };
}

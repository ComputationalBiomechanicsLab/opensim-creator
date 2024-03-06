#pragma once

#include <OpenSimCreator/Documents/ICustomDecorationGenerator.h>

#include <OpenSim/Simulation/Model/Geometry.h>
#include <oscar/Graphics/Mesh.h>

namespace osc
{
    // custom component for storing an in-memory mesh
    //
    // used for (e.g.) storing+showing warp results without having to persist
    // a mesh file to disk
    class InMemoryMesh : public OpenSim::Geometry, public ICustomDecorationGenerator {
        OpenSim_DECLARE_CONCRETE_OBJECT(InMemoryMesh, OpenSim::Geometry);
    public:
        InMemoryMesh() = default;
        explicit InMemoryMesh(Mesh const& mesh_) : m_OscMesh{mesh_} {}

        void implementCreateDecorativeGeometry(SimTK::Array_<SimTK::DecorativeGeometry>&) const override
        {
            // do nothing: this custom component uses the OSC-specific `ICustomDecorationGenerator` for perf
        }
    private:
        void implGenerateCustomDecorations(SimTK::State const&, std::function<void(SceneDecoration&&)> const&) const override;

        Mesh m_OscMesh;
    };
}

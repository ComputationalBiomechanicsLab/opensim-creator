#pragma once

#include <libopensimcreator/Documents/CustomComponents/ICustomDecorationGenerator.h>

#include <liboscar/Graphics/Mesh.h>
#include <OpenSim/Simulation/Model/Geometry.h>

namespace osc::mow
{
    // a custom `OpenSim::Geometry` that uses `osc::Mesh`es
    //
    // exists entirely for performance reasons: this enables the warping engine to produce
    // a renderable model in-memory without having to write `obj` files or similar (which is
    // required by `OpenSim::Mesh`)
    class InMemoryMesh : public OpenSim::Geometry, public ICustomDecorationGenerator {
        OpenSim_DECLARE_CONCRETE_OBJECT(InMemoryMesh, OpenSim::Geometry)
    public:
        InMemoryMesh() = default;
        explicit InMemoryMesh(const Mesh& mesh_) : m_OscMesh{mesh_} {}

        void implementCreateDecorativeGeometry(SimTK::Array_<SimTK::DecorativeGeometry>&) const override
        {
            // do nothing: OpenSim Creator will detect `ICustomDecorationDecorator` and use that
        }

        const Mesh& getOscMesh() const { return m_OscMesh; }
    private:
        void implGenerateCustomDecorations(const SimTK::State&, const std::function<void(SceneDecoration&&)>&) const override;

        Mesh m_OscMesh;
    };
}

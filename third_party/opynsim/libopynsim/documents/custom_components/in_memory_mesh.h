#pragma once

#include <libopynsim/documents/custom_components/custom_decoration_generator.h>

#include <liboscar/graphics/mesh.h>
#include <OpenSim/Simulation/Model/Geometry.h>

namespace opyn
{
    // a custom `OpenSim::Geometry` that uses `osc::Mesh`es
    //
    // exists entirely for performance reasons: this enables the warping engine to produce
    // a renderable model in-memory without having to write `obj` files or similar (which is
    // required by `OpenSim::Mesh`)
    class InMemoryMesh : public OpenSim::Geometry, public CustomDecorationGenerator {
        OpenSim_DECLARE_CONCRETE_OBJECT(InMemoryMesh, OpenSim::Geometry)
    public:
        InMemoryMesh() = default;
        explicit InMemoryMesh(const osc::Mesh& mesh_) : osc_mesh_{mesh_} {}

        void implementCreateDecorativeGeometry(SimTK::Array_<SimTK::DecorativeGeometry>&) const override
        {
            // do nothing: OpenSim Creator will detect `ICustomDecorationDecorator` and use that
        }

        const osc::Mesh& osc_mesh() const { return osc_mesh_; }
    private:
        void impl_generate_custom_decorations(const SimTK::State&, const std::function<void(osc::SceneDecoration&&)>&) const override;

        osc::Mesh osc_mesh_;
    };
}

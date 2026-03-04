#include "model_specification.h"

#include <libopynsim/utilities/open_sim_helpers.h>

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>

using namespace opyn;

namespace
{
    osc::CopyOnUpdPtr<OpenSim::Model> generate_example_pendulum()
    {
        auto rv = osc::make_cow<OpenSim::Model>();
        auto& model = *rv.upd();
        auto& rod1 = AddBody(model, "rod1", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{});
        auto& rod1_sphere = AttachGeometry<OpenSim::Sphere>(rod1, 0.05);
        rod1_sphere.setName("rod1_geom_2");
        auto& rod1_rod_frame = AddComponent<OpenSim::PhysicalOffsetFrame>(
            rod1,
            "rod1_geom_frame_1",
            rod1,
            SimTK::Transform{SimTK::Vec3{0.0, 0.25, 0.0}}
        );
        AttachGeometry<OpenSim::Cylinder>(rod1_rod_frame, 0.005, 0.25);
        auto& pin1 = AddJoint<OpenSim::PinJoint>(
            model,
            "pin1",
            model.getGround(),
            SimTK::Vec3{0.0, 1.1, 0.0},
            SimTK::Vec3{0.0},
            rod1,
            SimTK::Vec3{0.0, 0.5, 0.0},
            SimTK::Vec3{0.0}
        );
        pin1.updCoordinate().set_default_value(0.25*std::numbers::pi_v<double>);
        auto& rod2 = AddBody(model, "rod2", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{});
        auto& rod2_sphere = AttachGeometry<OpenSim::Sphere>(rod2, 0.05);
        rod2_sphere.setName("rod2_geom_2");
        auto& rod2_rod_frame = AddComponent<OpenSim::PhysicalOffsetFrame>(
            rod2,
            "rod2_geom_frame_1",
            rod2,
            SimTK::Transform{SimTK::Vec3{0.0, 0.25, 0.0}}
        );
        AttachGeometry<OpenSim::Cylinder>(rod2_rod_frame, 0.005, 0.25);
        auto& pin2 = AddJoint<OpenSim::PinJoint>(
            model,
            "pin2",
            rod1,
            SimTK::Vec3{0.0},
            SimTK::Vec3{0.0},
            rod2,
            SimTK::Vec3{0.0, 0.5, 0.0},
            SimTK::Vec3{0.0}
        );
        pin2.updCoordinate().set_default_value(0.25*std::numbers::pi_v<double>);

        // This must happen here, because the pointers (which reset/invalidate
        // on copy) must be written as component paths (copyable) before
        // the `ModelSpecification` is copied into a `opyn::Model`.
        model.finalizeConnections();
        return rv;
    }
}

class opyn::ModelSpecification::Impl final {
public:
    explicit Impl() = default;

    explicit Impl(const std::filesystem::path& osim_path) :
        model_{osc::make_cow<OpenSim::Model>(osim_path.string())}
    {}

    explicit Impl(osc::CopyOnUpdPtr<OpenSim::Model> model) :
        model_{std::move(model)}
    {}

    Model compile() const { return Model{*model_}; }
private:
    osc::CopyOnUpdPtr<OpenSim::Model> model_ = osc::make_cow<OpenSim::Model>();
};

opyn::ModelSpecification opyn::ModelSpecification::from_osim_file(const std::filesystem::path& osim_path)
{
    return ModelSpecification{osc::make_cow<Impl>(osim_path)};
}

opyn::ModelSpecification opyn::ModelSpecification::example_double_pendulum()
{
    return ModelSpecification{osc::make_cow<Impl>(generate_example_pendulum())};
}

opyn::ModelSpecification::ModelSpecification() :
    impl_{osc::make_cow<Impl>()}
{}

opyn::ModelSpecification::ModelSpecification(osc::CopyOnUpdPtr<Impl> impl) :
    impl_{std::move(impl)}
{}

Model opyn::ModelSpecification::compile() const { return impl_->compile(); }

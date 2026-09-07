#include "model_specification.h"

#include <libopynsim/documents/custom_components/in_memory_mesh.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/formats/obj.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <numbers>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace opyn;
namespace rgs = std::ranges;
namespace vws = std::views;

namespace
{
    osc::CopyOnUpdPtr<OpenSim::Model> generate_pendulum()
    {
        auto rv = osc::make_cow<OpenSim::Model>();
        auto& model = *rv.upd();

        // Setup head body with decorations
        auto& head = AddBody(model, "head", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
        auto& head_sphere_geom = AttachGeometry<OpenSim::Sphere>(head, 0.05);
        head_sphere_geom.setName("head_geom");
        auto& head_rod_pof = AddComponent<OpenSim::PhysicalOffsetFrame>(
            head,
            "head_rod_offset",
            head,
            SimTK::Transform{SimTK::Vec3{0.0, 0.25, 0.0}}
        );
        AttachGeometry<OpenSim::Cylinder>(head_rod_pof, 0.005, 0.25);

        // Attach body to ground with a pin joint
        auto& pin = AddJoint<OpenSim::PinJoint>(
            model,
            "pin",
            model.getGround(),
            SimTK::Vec3{0.0, 0.0, 0.0},
            SimTK::Vec3{0.0},
            head,
            SimTK::Vec3{0.0, -1.0, 0.0},
            SimTK::Vec3{0.0}
        );
        pin.updCoordinate().set_default_value(0.25*std::numbers::pi_v<double>);
        model.finalizeConnections();
        return rv;
    }

    osc::CopyOnUpdPtr<OpenSim::Model> generate_example_pendulum()
    {
        auto rv = osc::make_cow<OpenSim::Model>();
        auto& model = *rv.upd();
        auto& rod1 = AddBody(model, "rod1", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
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
        auto& rod2 = AddBody(model, "rod2", 1.0, SimTK::Vec3{0.0}, SimTK::Inertia{SimTK::Vec3{1.0}});
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

    explicit Impl(const std::filesystem::path& source) :
        model_{osc::make_cow<OpenSim::Model>(source.string())}
    {}
    explicit Impl(OpenSim::Model&& opensim_model) :
        model_{osc::make_cow<OpenSim::Model>(std::move(opensim_model))}
    {
        InitializeModel(*model_.upd());
    }
    explicit Impl(osc::CopyOnUpdPtr<OpenSim::Model> model) :
        model_{std::move(model)}
    {}

    Model compile() const { return Model{*model_}; }

    std::filesystem::path root_directory() const
    {
        const std::string& ifn = model_->getInputFileName();

        if (ifn.empty() or ifn == "Unassigned") {
            return std::filesystem::path{"."};
        } else {
            return std::filesystem::path{ifn}.parent_path();
        }
    }

    void set_root_directory(const std::filesystem::path& directory)
    {
        OpenSim::Model& model = *model_.upd();
        const std::string& ifn = model.getInputFileName();

        if (ifn.empty() or ifn == "Unassigned") {
            model.setInputFileName((directory / "untitled.osim").string());
        } else {
            model.setInputFileName((directory / std::filesystem::path{ifn}.filename()).string());
        }
    }

    void to_osim(const std::filesystem::path& destination) const
    {
        assert_model_contains_no_in_memory_resources();
        model_->print(destination.string());
    }

    std::string to_osim() const
    {
        assert_model_contains_no_in_memory_resources();
        return model_->dump();
    }

    void bake_station_defined_frames()
    {
        BakeStationDefinedFrames(*model_.upd());
    }

    void flush_in_memory_resources_to(const std::filesystem::path& directory)
    {
        // Pre-pass: collect all `InMemoryMesh` absolute paths so that the replacement
        // code does not need to iterate the model tree while it mutates it.
        std::vector<OpenSim::ComponentPath> imm_abs_paths;
        for (const auto& in_memory_mesh : model_->getComponentList<InMemoryMesh>()) {
            imm_abs_paths.push_back(in_memory_mesh.getAbsolutePath());
        }

        if (imm_abs_paths.empty()) {
            return;  // No `InMemoryMesh`es in the model: exit early.
        }

        // Assert all `InMemoryMesh`es have unique names.
        {
            std::unordered_map<std::string, size_t> name_counts;
            for (const auto& imm_abs_path : imm_abs_paths) {
                name_counts[imm_abs_path.getComponentName()]++;
            }
            const auto count_gt_1 = [](const auto& p) { return p.second > 1; };
            if (rgs::any_of(name_counts, count_gt_1)) {
                std::string error_message = "Cannot flush in-memory resources: the `ModelSpecification` the following in-memory components have duplicate names:";
                for (const auto& [name, count] : name_counts | vws::filter(count_gt_1)) {
                    std::format_to(std::back_inserter(error_message), "- {} ({} occurrences)", name, count);
                }
                throw std::runtime_error{error_message};
            }
        }

        // Ensure the output directory exists.
        if (not std::filesystem::exists(root_directory() / directory)) {
            std::filesystem::create_directories(root_directory() / directory);
        }

        // Perform model update.
        OpenSim::Model& mutable_model = *model_.upd();
        for (const auto& imm_abs_path : imm_abs_paths) {
            std::filesystem::path filename{imm_abs_path.getComponentName()};
            filename.replace_extension(".obj");
            const std::filesystem::path filesystem_path = root_directory() / directory / filename;
            const std::filesystem::path property_path   = (directory / filename).lexically_normal().generic_string();

            // Write in-memory warped mesh data to disk as an OBJ file.
            auto& imm = mutable_model.updComponent<InMemoryMesh>(imm_abs_path);
            {
                std::ofstream obj_stream{filesystem_path, std::ios::trunc};
                obj_stream.exceptions(std::ios::badbit | std::ios::failbit);
                osc::OBJ::write(obj_stream, imm.getOscMesh(), osc::OBJMetadata{"osc-model-warper"});
            }

            // Replace `InMemoryMesh` with a standard `OpenSim::Mesh`.
            auto opensim_mesh = std::make_unique<OpenSim::Mesh>();
            opensim_mesh->set_mesh_file(property_path.string());
            OverwriteGeometry(mutable_model, imm, std::move(opensim_mesh));
        }

        // Ensure mutated model is up-to-date etc.
        InitializeModel(mutable_model);
    }

private:
    void assert_model_contains_no_in_memory_resources() const
    {
        std::vector<const InMemoryMesh*> violations;
        for (const auto& in_memory_mesh : model_->getComponentList<InMemoryMesh>()) {
            violations.push_back(&in_memory_mesh);
        }
        if (not violations.empty()) {
            std::string error_msg = "Cannot serialize `ModelSpecification`: it contains the following in-memory resources:";
            for (const InMemoryMesh* in_memory_mesh : violations) {
                std::format_to(std::back_inserter(error_msg), "- {}", in_memory_mesh->getAbsolutePathString());
            }
            error_msg += "You can fix this by first flushing them to disk with `model_specification.flush_in_memory_resources_to(directory)`. If you are using a relative `directory`, you may also need to set the `root_path` of this model to the directory where it will be stored.";
            throw std::runtime_error{error_msg};
        }
    }

    osc::CopyOnUpdPtr<OpenSim::Model> model_ = osc::make_cow<OpenSim::Model>();
};

opyn::ModelSpecification opyn::ModelSpecification::from_osim(const std::filesystem::path& source)
{
    return ModelSpecification{osc::make_cow<Impl>(source)};
}

opyn::ModelSpecification opyn::ModelSpecification::example_pendulum()
{
    return ModelSpecification{osc::make_cow<Impl>(generate_pendulum())};
}

opyn::ModelSpecification opyn::ModelSpecification::example_double_pendulum()
{
    return ModelSpecification{osc::make_cow<Impl>(generate_example_pendulum())};
}

opyn::ModelSpecification::ModelSpecification() :
    impl_{osc::make_cow<Impl>()}
{}
opyn::ModelSpecification::ModelSpecification(OpenSim::Model&& opensim_model) :
    impl_{osc::make_cow<Impl>(std::move(opensim_model))}
{}
opyn::ModelSpecification::ModelSpecification(osc::CopyOnUpdPtr<Impl> impl) :
    impl_{std::move(impl)}
{}

Model opyn::ModelSpecification::compile() const { return impl_->compile(); }
std::filesystem::path opyn::ModelSpecification::root_directory() const { return impl_->root_directory(); }
void opyn::ModelSpecification::set_root_directory(const std::filesystem::path& directory) { impl_.upd()->set_root_directory(directory); }
void opyn::ModelSpecification::to_osim(const std::filesystem::path& destination) const { impl_->to_osim(destination); }
std::string opyn::ModelSpecification::to_osim() const { return impl_->to_osim(); }
void opyn::ModelSpecification::bake_station_defined_frames()
{
    impl_.upd()->bake_station_defined_frames();
}
void opyn::ModelSpecification::flush_in_memory_resources_to(const std::filesystem::path& directory)
{
    impl_.upd()->flush_in_memory_resources_to(directory);
}

#pragma once

#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_scaling_step.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/filesystem_helpers.h>

#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace OpenSim
{
    class Model;
}

namespace opyn
{
    // A `ThinPlateSpline` scaling step that substitutes a single new mesh in place of the
    // original one by using the TPS translation + rotation terms to reorient/scale the mesh
    // correctly.
    class ThinPlateSplineMeshSubstitutionScalingStep final : public ThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(
            ThinPlateSplineMeshSubstitutionScalingStep, ThinPlateSplineScalingStep)

    public:
        OpenSim_DECLARE_PROPERTY(
            source_mesh_component_path,
            std::string,
            "Absolute path (e.g. `/bodyset/body/geom_1`) to a mesh component in the model that should be substituted.");
        OpenSim_DECLARE_PROPERTY(
            destination_mesh_file,
            std::string,
            "Filesystem path, potentially relative to the model file, to a mesh file that should be warped + substitute 'source_mesh_component_path'");

        explicit ThinPlateSplineMeshSubstitutionScalingStep() :
            ThinPlateSplineScalingStep{
                "Substitute Mesh via Inverse Thin-Plate Spline (TPS) Affine Transform"}
        {
            setDescription(
                "Substitutes the source mesh in the model with a new mesh file. The mesh's affine rotation and translation (i.e. frame) will be computed from the inverse of the Thin-Plate Spline (TPS) between the source and destination landmarks (i.e. it'll use them to figure out how to go _from_ the destination mesh coordinate system _to_ the source mesh coordinate system in a way that doesn't rescale or warp the destination mesh).");
            constructProperty_source_mesh_component_path("");
            constructProperty_destination_mesh_file("");
        }

        std::vector<ScalingStepValidationMessage> impl_validate(
            ScalingCache& scaling_cache,
            const ScalingParameters& parameters,
            const OpenSim::Model& source_model) const override
        {
            auto messages =
                ThinPlateSplineScalingStep::impl_validate(scaling_cache, parameters, source_model);

            // Ensure the model has a filesystem location (prerequisite).
            const auto model_filesystem_location = try_find_input_file(source_model);
            if (not model_filesystem_location) {
                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    "The source model has no filesystem location.");
                return messages;
            }

            // Ensure the `destination_mesh_filepath` can be found (relative to the model osim).
            if (get_destination_mesh_file().empty()) {
                messages.emplace_back(
                    ScalingStepValidationState::Error, "`destination_mesh_file` is empty.");
            } else if (const auto destination_mesh_path =
                           model_filesystem_location->parent_path() /
                           osc::read_windows_or_unix_path_string(get_destination_mesh_file());
                       not std::filesystem::exists(destination_mesh_path)) {

                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    std::format(
                        "{}: : Cannot find `destination_mesh_file` on filesystem",
                        destination_mesh_path.string()));
            }

            // Ensure `source_mesh_component_path` exists in the model
            const auto* source_mesh =
                find_component<OpenSim::Mesh>(source_model, get_source_mesh_component_path());
            if (not source_mesh) {
                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    std::format(
                        "{}: Cannot find Mesh 'source_mesh_component_path' in the source model (or it isn't a Mesh).",
                        get_source_mesh_component_path()));
            }

            return messages;
        }

        void impl_apply_scaling_step(
            ScalingCache& cache,
            const ScalingParameters& parameters,
            const OpenSim::Model& source_model,
            OpenSim::Model& result_model) const override
        {
            // Lookup/validate warping inputs.
            const auto common_params =
                calc_tps_scaling_step_common_params(parameters, source_model, result_model);

            // Lookup/validate warping inputs.
            const std::optional<std::filesystem::path> model_filesystem_location =
                try_find_input_file(result_model);
            OSC_ASSERT_ALWAYS(
                model_filesystem_location && "The source model has no filesystem location");

            OSC_ASSERT_ALWAYS(not get_destination_mesh_file().empty());
            const std::filesystem::path destination_mesh_path =
                model_filesystem_location->parent_path() /
                osc::read_windows_or_unix_path_string(get_destination_mesh_file());

            OSC_ASSERT_ALWAYS(not get_source_mesh_component_path().empty());
            const auto* source_mesh =
                find_component<OpenSim::Geometry>(result_model, get_source_mesh_component_path());
            OSC_ASSERT_ALWAYS(
                (dynamic_cast<const OpenSim::Mesh*>(source_mesh) or
                 dynamic_cast<const InMemoryMesh*>(source_mesh)) &&
                "'source_mesh_component_path' exists in the model but isn't mesh-like");
            OSC_ASSERT_ALWAYS(
                source_mesh && "could not find `source_mesh_component_path` in the model");

            const SimTK::Transform t =
                cache.lookup_tps_affine_transform_without_scaling(common_params.tps_inputs);
            const SimTK::Vec3 new_scale_factors =
                (common_params.tps_inputs.destination_landmarks_prescale /
                 common_params.tps_inputs.source_landmarks_prescale) *
                source_mesh->get_scale_factors();

            // Find existing mesh
            auto* destination_mesh = find_component_mut<OpenSim::Geometry>(
                result_model, get_source_mesh_component_path());
            OSC_ASSERT_ALWAYS(
                destination_mesh &&
                "could not find `source_mesh_component_path` in the result model");
            OSC_ASSERT_ALWAYS(
                (dynamic_cast<const OpenSim::Mesh*>(destination_mesh) or
                 dynamic_cast<const InMemoryMesh*>(destination_mesh)) &&
                "'source_mesh_component_path' exists in the model but isn't mesh-like");
            const std::string existing_mesh_name = destination_mesh->getName();
            // Figure out existing mesh's frame
            auto* old_parent_frame = find_component_mut<OpenSim::PhysicalFrame>(
                result_model, destination_mesh->getFrame().getAbsolutePath());
            OSC_ASSERT_ALWAYS(old_parent_frame);
            // Delete existing mesh from model
            OSC_ASSERT_ALWAYS(try_delete_component_from_model(result_model, *destination_mesh));
            initialize_model(result_model);
            initialize_state(result_model);

            // Add new frame + mesh to the model
            auto* new_frame = new OpenSim::PhysicalOffsetFrame{*old_parent_frame, t.invert()};
            new_frame->updSocket("parent").setConnecteePath(
                old_parent_frame->getAbsolutePathString());
            result_model.addComponent(new_frame);

            auto new_mesh = std::make_unique<OpenSim::Mesh>();
            new_mesh->setName(existing_mesh_name);
            new_mesh->set_scale_factors(new_scale_factors);
            new_mesh->set_mesh_file(destination_mesh_path.string());
            new_mesh->updSocket("frame").setConnecteePath(new_frame->getAbsolutePathString());
            result_model.addComponent(new_mesh.release());

            initialize_model(result_model);
            initialize_state(result_model);
        }
    };
}

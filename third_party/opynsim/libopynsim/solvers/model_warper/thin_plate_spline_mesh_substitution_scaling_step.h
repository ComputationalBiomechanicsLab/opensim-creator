#pragma once

#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_scaling_step.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <liboscar/utilities/assertions.h>

#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace OpenSim { class Model; }

namespace opyn
{
    // A `ThinPlateSpline` scaling step that substitutes a single new mesh in place of the
    // original one by using the TPS translation + rotation terms to reorient/scale the mesh
    // correctly.
    class ThinPlateSplineMeshSubstitutionScalingStep final : public ThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineMeshSubstitutionScalingStep, ThinPlateSplineScalingStep)
    public:
        OpenSim_DECLARE_PROPERTY(source_mesh_component_path, std::string, "Absolute path (e.g. `/bodyset/body/geom_1`) to a mesh component in the model that should be substituted.");
        OpenSim_DECLARE_PROPERTY(destination_mesh_file, std::string, "Filesystem path, potentially relative to the model file, to a mesh file that should be warped + substitute 'source_mesh_component_path'");

        explicit ThinPlateSplineMeshSubstitutionScalingStep() :
            ThinPlateSplineScalingStep{"Substitute Mesh via Inverse Thin-Plate Spline (TPS) Affine Transform"}
        {
            setDescription("Substitutes the source mesh in the model with a new mesh file. The mesh's affine rotation and translation (i.e. frame) will be computed from the inverse of the Thin-Plate Spline (TPS) between the source and destination landmarks (i.e. it'll use them to figure out how to go _from_ the destination mesh coordinate system _to_ the source mesh coordinate system in a way that doesn't rescale or warp the destination mesh).");
            constructProperty_source_mesh_component_path("");
            constructProperty_destination_mesh_file("");
        }

        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache& scalingCache,
            const ScalingParameters& parameters,
            const OpenSim::Model& sourceModel) const final
        {
            auto messages = ThinPlateSplineScalingStep::implValidate(scalingCache, parameters, sourceModel);

            // Ensure the model has a filesystem location (prerequisite).
            const auto modelFilesystemLocation = TryFindInputFile(sourceModel);
            if (not modelFilesystemLocation) {
                messages.emplace_back(ScalingStepValidationState::Error, "The source model has no filesystem location.");
                return messages;
            }

            // Ensure the `destination_mesh_filepath` can be found (relative to the model osim).
            if (get_destination_mesh_file().empty()) {
                messages.emplace_back(ScalingStepValidationState::Error, "`destination_mesh_file` is empty.");
            }
            else if (const auto destinationMeshPath = modelFilesystemLocation->parent_path() / get_destination_mesh_file();
                not std::filesystem::exists(destinationMeshPath)) {

                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    std::format("{}: : Cannot find `destination_mesh_file` on filesystem", destinationMeshPath.string())
                );
            }

            // Ensure `source_mesh_component_path` exists in the model
            const auto* sourceMesh = FindComponent<OpenSim::Mesh>(sourceModel, get_source_mesh_component_path());
            if (not sourceMesh) {
                messages.emplace_back(
                    ScalingStepValidationState::Error,
                    std::format("{}: Cannot find Mesh 'source_mesh_component_path' in the source model (or it isn't a Mesh).", get_source_mesh_component_path())
                );
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache& cache,
            const ScalingParameters& parameters,
            const OpenSim::Model& sourceModel,
            OpenSim::Model& resultModel) const final
        {
            // Lookup/validate warping inputs.
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel, resultModel);

            // Lookup/validate warping inputs.
            const std::optional<std::filesystem::path> modelFilesystemLocation = TryFindInputFile(resultModel);
            OSC_ASSERT_ALWAYS(modelFilesystemLocation && "The source model has no filesystem location");

            OSC_ASSERT_ALWAYS(not get_destination_mesh_file().empty());
            const std::filesystem::path destinationMeshPath = modelFilesystemLocation->parent_path() / get_destination_mesh_file();

            OSC_ASSERT_ALWAYS(not get_source_mesh_component_path().empty());
            const auto* sourceMesh = FindComponent<OpenSim::Geometry>(resultModel, get_source_mesh_component_path());
            OSC_ASSERT_ALWAYS((dynamic_cast<const OpenSim::Mesh*>(sourceMesh) or dynamic_cast<const InMemoryMesh*>(sourceMesh)) && "'source_mesh_component_path' exists in the model but isn't mesh-like");
            OSC_ASSERT_ALWAYS(sourceMesh && "could not find `source_mesh_component_path` in the model");

            // TODO: everything below is a hack because manipulating models
            // via OpenSim is like pulling teeth, underwater, from a shark,
            // with rabies, using your bare hands.

            const SimTK::Transform t = cache.lookupTPSAffineTransformWithoutScaling(commonParams.tpsInputs);
            const SimTK::Vec3 newScaleFactors =
                (commonParams.tpsInputs.destinationLandmarksPrescale/commonParams.tpsInputs.sourceLandmarksPrescale) * sourceMesh->get_scale_factors();

            // Find existing mesh
            auto* destinationMesh = FindComponentMut<OpenSim::Geometry>(resultModel, get_source_mesh_component_path());
            OSC_ASSERT_ALWAYS(destinationMesh && "could not find `source_mesh_component_path` in the result model");
            OSC_ASSERT_ALWAYS((dynamic_cast<const OpenSim::Mesh*>(destinationMesh) or dynamic_cast<const InMemoryMesh*>(destinationMesh)) && "'source_mesh_component_path' exists in the model but isn't mesh-like");
            const std::string existingMeshName = destinationMesh->getName();
            // Figure out existing mesh's frame
            auto* oldParentFrame = FindComponentMut<OpenSim::PhysicalFrame>(resultModel, destinationMesh->getFrame().getAbsolutePath());
            OSC_ASSERT_ALWAYS(oldParentFrame);
            // Delete existing mesh from model
            OSC_ASSERT_ALWAYS(TryDeleteComponentFromModel(resultModel, *destinationMesh));
            InitializeModel(resultModel);
            InitializeState(resultModel);

            // Add new frame + mesh to the model
            auto* newFrame = new OpenSim::PhysicalOffsetFrame{*oldParentFrame, t.invert()};
            newFrame->updSocket("parent").setConnecteePath(oldParentFrame->getAbsolutePathString());
            resultModel.addComponent(newFrame);

            auto newMesh = std::make_unique<OpenSim::Mesh>();
            newMesh->setName(existingMeshName);
            newMesh->set_scale_factors(newScaleFactors);
            newMesh->set_mesh_file(destinationMeshPath.string());
            newMesh->updSocket("frame").setConnecteePath(newFrame->getAbsolutePathString());
            resultModel.addComponent(newMesh.release());

            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };
}

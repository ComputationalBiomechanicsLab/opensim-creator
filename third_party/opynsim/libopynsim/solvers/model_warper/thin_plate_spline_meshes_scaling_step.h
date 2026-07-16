#pragma once

#include <libopynsim/documents/custom_components/in_memory_mesh.h>
#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameters.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/solvers/model_warper/toggleable_thin_plate_spline_scaling_step.h>
#include <libopynsim/utilities/open_sim_helpers.h>

#include <OpenSim/Simulation/Model/Geometry.h>

#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace opyn
{
    // A `ScalingStep` that warps `OpenSim::Mesh`es in the source model by using
    // the Thin-Plate Spline (TPS) warping algorithm on landmark pairs loaded from
    // associated files.
    class ThinPlateSplineMeshesScalingStep final : public ToggleableThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineMeshesScalingStep, ToggleableThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(meshes, std::string, "Component path(s), relative to the model, that locates mesh(es) that should be scaled by this scaling step (e.g. `/bodyset/torso/torso_geom_4`)");

    public:
        explicit ThinPlateSplineMeshesScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to Meshes"}
        {
            setDescription("Warps mesh(es) in the source model by applying a Thin-Plate Spline (TPS) warp to each vertex in the souce mesh(es).");
            constructProperty_meshes();
        }
    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache& cache,
            const ScalingParameters& params,
            const OpenSim::Model& sourceModel) const final
        {
            // Get base class validation messages.
            auto messages = ToggleableThinPlateSplineScalingStep::implValidate(cache, params, sourceModel);

            // Ensure at least one mesh is specified.
            if (getProperty_meshes().empty()) {
                messages.emplace_back(ScalingStepValidationState::Error, "No mesh(es) given (e.g. `/bodyset/torso/torso_geom`).");
            }

            // Ensure all specified meshes can be found in the source model.
            for (int i = 0; i < getProperty_meshes().size(); ++i) {
                const auto* mesh = FindComponent<OpenSim::Mesh>(sourceModel, get_meshes(i));
                if (not mesh) {
                    messages.emplace_back(
                        ScalingStepValidationState::Error,
                        std::format("{}: Cannot find entry in 'meshes' in the source model (or it isn't a Mesh).", get_meshes(i))
                    );
                }
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache& scalingCache,
            const ScalingParameters& parameters,
            const OpenSim::Model& sourceModel,
            OpenSim::Model& resultModel) const final
        {
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel, resultModel);

            // Warp each mesh specified by the `meshes` property.
            for (int i = 0; i < getProperty_meshes().size(); ++i) {
                const auto* sourceMesh = FindComponent<OpenSim::Mesh>(sourceModel, get_meshes(i));
                OSC_ASSERT_ALWAYS(sourceMesh && "could not find a mesh in the source model");

                // Find the input mesh and use it produce the warped mesh.
                const auto* resultMesh = FindComponent<OpenSim::Mesh>(resultModel, get_meshes(i));
                OSC_ASSERT_ALWAYS(resultMesh && "could not find a mesh in the model");

                std::unique_ptr<InMemoryMesh> warpedMesh = scalingCache.lookupTPSMeshWarp(
                    sourceModel,
                    resultModel,
                    *sourceMesh,
                    *resultMesh,
                    *commonParams.sourceLandmarksFrame,
                    *commonParams.resultLandmarksFrame,
                    commonParams.tpsInputs,
                    commonParams.compensateForFrameChanges
                );
                OSC_ASSERT_ALWAYS(warpedMesh && "warping a mesh in the model failed");

                // Overwrite the mesh in the result model with the warped mesh.
                auto* resultMeshMut = FindComponentMut<OpenSim::Mesh>(resultModel, get_meshes(i));
                OSC_ASSERT_ALWAYS(resultMesh && "could not find a corresponding mesh in the result model");
                OverwriteGeometry(resultModel, *resultMeshMut, std::move(warpedMesh));
                OSC_ASSERT_ALWAYS(FindComponent<InMemoryMesh>(resultModel, get_meshes(i)) != nullptr);
            }
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };
}

#pragma once

#include <libopynsim/solvers/model_warper/scaling_cache.h>
#include <libopynsim/solvers/model_warper/scaling_parameter_declaration.h>
#include <libopynsim/solvers/model_warper/scaling_step.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_message.h>
#include <libopynsim/solvers/model_warper/scaling_step_validation_state.h>
#include <libopynsim/solvers/model_warper/thin_plate_spline_common_inputs.h>

#include <liboscar/utilities/assertions.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace opyn
{
    // An abstract base class for a `ScalingStep` that uses the Thin-Plate Spline (TPS)
    // algorithm to perform some kind of (concrete-implementation-defined) scaling to
    // the model.
    class ThinPlateSplineScalingStep : public ScalingStep {
        OpenSim_DECLARE_ABSTRACT_OBJECT(ThinPlateSplineScalingStep, ScalingStep)

    public:
        // These parameters are common for all TPS `ScalingStep`s.
        OpenSim_DECLARE_PROPERTY(source_landmarks_file, std::string, "Filesystem path, relative to the model's filesystem path, where a CSV containing the source landmarks can be loaded from (e.g. `Geometry/torso.landmarks.csv`)");
        OpenSim_DECLARE_PROPERTY(destination_landmarks_file, std::string, "Filesystem path, relative to the model's filesystem path, where a CSV containing the destination landmarks can be loaded from (e.g. `DestinationGeometry/torso.landmarks.csv`)");
        OpenSim_DECLARE_PROPERTY(landmarks_frame, std::string, "Component path (e.g. `/bodyset/somebody`) to the frame that the landmarks defined in both `source_landmarks_file` and `destination_landmarks_file` are expressed in.\n\nThe engine uses this to figure out how to transform the input to/from the coordinate system of the warp transform.");
        OpenSim_DECLARE_PROPERTY(compensate_for_frame_changes, bool, "If `landmarks_frame` is different from the source data's frame, and previous scaling steps have caused the spatial transform between those two frames to change, compensate for it by inverse-applying the difference between the frames to the result, so that the effect of those frame changes is compensated for. This can be necessary to stop double-warping for occurring (e.g. when separately warping a frame followed by warping data within that frame)");
        OpenSim_DECLARE_PROPERTY(source_landmarks_prescale, double, "Scaling factor that each source landmark point should be multiplied by before computing the TPS warp. This is sometimes necessary if (e.g.) the mesh is in different units (OpenSim works in meters).");
        OpenSim_DECLARE_PROPERTY(destination_landmarks_prescale, double, "Scaling factor that each destination landmark point should be multiplied by before computing the TPS warp. This is sometimes necessary if (e.g.) the mesh is in different units (OpenSim works in meters).");
        OpenSim_DECLARE_PROPERTY(bending_penalty, double, "A bending penalty that smooths out the warp. Explained in OPynSim's Thin-Plate Spline documentation.");

        struct CommonParameters final {
            ThinPlateSplineCommonInputs tpsInputs;
            const OpenSim::Frame* sourceLandmarksFrame = nullptr;
            const OpenSim::Frame* resultLandmarksFrame = nullptr;
            bool compensateForFrameChanges = false;
        };

    protected:

        // Constructs a `ThinPlateSplineScalingStep` with defaulted parameters.
        explicit ThinPlateSplineScalingStep(std::string_view label) :
            ScalingStep{label}
        {
            constructProperty_source_landmarks_file({});
            constructProperty_destination_landmarks_file({});
            constructProperty_landmarks_frame("/ground");
            constructProperty_compensate_for_frame_changes(false);
            constructProperty_source_landmarks_prescale(1.0);
            constructProperty_destination_landmarks_prescale(1.0);
            constructProperty_bending_penalty(0.0);
        }

        // Overriders should still call this base method.
        void implForEachScalingParameterDeclaration(const std::function<void(const ScalingParameterDeclaration&)>& callback) const override
        {
            callback(ScalingParameterDeclaration{"blending_factor", 1.0});
        }

        // Performs validation steps that are common for all TPS scaling steps, so overriders
        // should call this base method.
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model& sourceModel) const override
        {
            std::vector<ScalingStepValidationMessage> messages;

            // Ensure the model has a filesystem location (prerequisite).
            const auto modelFilesystemLocation = opyn::TryFindInputFile(sourceModel);
            if (not modelFilesystemLocation) {
                messages.emplace_back(ScalingStepValidationState::Error, "The source model has no filesystem location.");
                return messages;
            }

            // Ensure the `source_landmarks_file` can be found (relative to the model osim).
            if (get_source_landmarks_file().empty()) {
                messages.emplace_back(ScalingStepValidationState::Error, "`source_landmarks_file` is empty.");
            }
            else if (const auto sourceLandmarksPath = modelFilesystemLocation->parent_path() / get_source_landmarks_file();
                not std::filesystem::exists(sourceLandmarksPath)) {

                std::stringstream msg;
                msg << sourceLandmarksPath.string() << ": Cannot find source landmarks file on filesystem";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            // Ensure the `destination_landmarks_file` can be found (relative to the model osim).
            if (get_destination_landmarks_file().empty()) {
                messages.emplace_back(ScalingStepValidationState::Error, "`destination_landmarks_file` is empty.");
            }
            else if (const auto destinationLandmarksPath = modelFilesystemLocation->parent_path() / get_destination_landmarks_file();
                not std::filesystem::exists(destinationLandmarksPath)) {

                std::stringstream msg;
                msg << destinationLandmarksPath.string() << ": Cannot find destination landmarks file on filesystem";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            // Ensure `landmarks_frame` exists in the model
            const auto* landmarksFrame = FindComponent<OpenSim::Frame>(sourceModel, get_landmarks_frame());
            if (not landmarksFrame) {
                std::stringstream msg;
                msg << get_landmarks_frame() << ": Cannot find this frame in the source model (or it isn't a Frame).";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            return messages;
        }

        // Returns TPS `ScalingStep` parameters that are common for all uses of the TPS
        // algorithm.
        CommonParameters calcTPSScalingStepCommonParams(
            const ScalingParameters& parameters,
            const OpenSim::Model& sourceModel,
            const OpenSim::Model& resultModel) const
        {
            // Lookup/validate warping inputs.
            const std::optional<std::filesystem::path> modelFilesystemLocation = TryFindInputFile(resultModel);
            OSC_ASSERT_ALWAYS(modelFilesystemLocation && "The source model has no filesystem location");

            OSC_ASSERT_ALWAYS(not get_source_landmarks_file().empty());
            const std::filesystem::path sourceLandmarksPath = modelFilesystemLocation->parent_path() / get_source_landmarks_file();

            OSC_ASSERT_ALWAYS(not get_destination_landmarks_file().empty());
            const std::filesystem::path destinationLandmarksPath = modelFilesystemLocation->parent_path() / get_destination_landmarks_file();

            OSC_ASSERT_ALWAYS(not get_landmarks_frame().empty());

            const auto* sourceLandmarksFrame = FindComponent<OpenSim::Frame>(sourceModel, get_landmarks_frame());
            OSC_ASSERT_ALWAYS(sourceLandmarksFrame && "could not find the landmarks frame in the source model");

            const auto* resultLandmarksFrame = FindComponent<OpenSim::Frame>(resultModel, get_landmarks_frame());
            OSC_ASSERT_ALWAYS(resultLandmarksFrame && "could not find the landmarks frame in the model");

            const std::optional<double> blendingFactor = parameters.lookup<double>("blending_factor");
            OSC_ASSERT_ALWAYS(blendingFactor && "blending_factor was not set by the warping engine");

            return CommonParameters{
                .tpsInputs = ThinPlateSplineCommonInputs{
                    sourceLandmarksPath,
                    destinationLandmarksPath,
                    get_source_landmarks_prescale(),
                    get_destination_landmarks_prescale(),
                    *blendingFactor,
                    get_bending_penalty(),
                },
                .sourceLandmarksFrame = sourceLandmarksFrame,
                .resultLandmarksFrame = resultLandmarksFrame,
                .compensateForFrameChanges = get_compensate_for_frame_changes(),
            };
        }
    };
}

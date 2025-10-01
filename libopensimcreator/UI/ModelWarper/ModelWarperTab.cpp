#include "ModelWarperTab.h"

#include <libopensimcreator/Documents/CustomComponents/InMemoryMesh.h>
#include <libopensimcreator/Documents/FileFilters.h>
#include <libopensimcreator/Documents/Landmarks/LandmarkHelpers.h>
#include <libopensimcreator/Documents/Landmarks/MaybeNamedLandmarkPair.h>
#include <libopensimcreator/Documents/Model/BasicModelStatePair.h>
#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Documents/Model/UndoableModelActions.h>
#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Graphics/OpenSimDecorationGenerator.h>
#include <libopensimcreator/Platform/IconCodepoints.h>
#include <libopensimcreator/Platform/RecentFiles.h>
#include <libopensimcreator/UI/ModelEditor/ModelEditorTab.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/UI/Shared/MainMenu.h>
#include <libopensimcreator/UI/Shared/ModelViewerPanel.h>
#include <libopensimcreator/UI/Shared/ModelViewerPanelParameters.h>
#include <libopensimcreator/UI/Shared/ObjectPropertiesEditor.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>
#include <libopensimcreator/Utils/SimTKConverters.h>
#include <libopensimcreator/Utils/TPS3D.h>

#include <liboscar/oscar.h>
#include <OpenSim/Simulation/Model/Frame.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Wrap/WrapCylinder.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace osc;
using namespace osc::mow;

// Scaling document related functions/datastructures.
namespace
{
    // A single, potentially user-provided, scaling parameter.
    //
    // It is the responsibility of the engine/UI to gather/provide this to the
    // scaling engine at scale-time.
    using ScalingParameterValue = double;

    // A declaration of a scaling parameter.
    //
    // `ScalingStep`s can declare that they may/must use a named `ScalingParameterValue`s
    // at runtime. This class is how they express that requirement. It's the scaling
    // engine/UI's responsibility to provide `ScalingParameters` that satisfy all
    // `ScalingStep`'s declarations.
    class ScalingParameterDeclaration final {
    public:
        explicit ScalingParameterDeclaration(std::string name, ScalingParameterValue defaultValue) :
            m_Name{std::move(name)},
            m_DefaultValue{defaultValue}
        {}

        const std::string& name() const { return m_Name; }
        const ScalingParameterValue& default_value() const { return m_DefaultValue; }
    private:
        std::string m_Name;
        ScalingParameterValue m_DefaultValue;
    };

    // A scaling parameter value override, which is usually provided by the top-level
    // document to override the default that a `ScalingStep` would typically declare
    // in a `ScalingParameterDeclaration`.
    class ScalingParameterOverride final : public OpenSim::Object {
        OpenSim_DECLARE_CONCRETE_OBJECT(ScalingParameterOverride, OpenSim::Object)
    public:
        OpenSim_DECLARE_PROPERTY(parameter_name, std::string, "The name of the scaling parameter that should have its default overridden.");
        OpenSim_DECLARE_PROPERTY(parameter_value, ScalingParameterValue, "The value to override the scaling parameter with. Note: it must have the correct datatype for the given scaling parameter.");

        explicit ScalingParameterOverride()
        {
            constructProperty_parameter_name("unknown");
            constructProperty_parameter_value(1.0);
        }

        explicit ScalingParameterOverride(const std::string& name, ScalingParameterValue value)
        {
            constructProperty_parameter_name(name);
            constructProperty_parameter_value(value);
        }
    };

    // A collection of runtime scaling parameters, usually created by aggregating
    // from individual `ScalingStep`s and `ScalingParameterOverride`s.
    class ScalingParameters final {
    public:
        template<std::same_as<double> T>
        std::optional<T> lookup(const std::string& key) const
        {
            const auto it = m_Values.find(key);
            if (it == m_Values.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        auto begin() const { return m_Values.begin(); }
        auto end() const { return m_Values.end(); }

        auto try_emplace(const std::string& name, const ScalingParameterValue& value)
        {
            return m_Values.try_emplace(name, value);
        }

        auto insert_or_assign(const std::string& name, const ScalingParameterValue& value)
        {
            return m_Values.insert_or_assign(name, value);
        }
    private:
        std::map<std::string, ScalingParameterValue> m_Values;
    };

    // Struct of runtime inputs that are common for all uses of the TPS
    // scaling algorithm.
    struct ThinPlateSplineCommonInputs final {

        explicit ThinPlateSplineCommonInputs(
            std::filesystem::path sourceLandmarksPath_,
            std::filesystem::path destinationLandmarksPath_,
            double sourceLandmarksPrescale_,
            double destinationLandmarksPrescale_,
            double blendingFactor_) :

            sourceLandmarksPath{std::move(sourceLandmarksPath_)},
            destinationLandmarksPath{std::move(destinationLandmarksPath_)},
            sourceLandmarksPrescale{sourceLandmarksPrescale_},
            destinationLandmarksPrescale{destinationLandmarksPrescale_},
            blendingFactor{blendingFactor_}
        {}

        std::filesystem::path sourceLandmarksPath;
        std::filesystem::path destinationLandmarksPath;
        double sourceLandmarksPrescale;
        double destinationLandmarksPrescale;
        bool applyAffineTranslation = true;
        bool applyAffineScale = true;
        bool applyAffineRotation = true;
        bool applyNonAffineWarp = true;
        double blendingFactor = 1.0;
    };

    // A cache that is (presumed to be) persisted between multiple executions of the
    // model warping pipeline, in order to improve runtime performance.
    class ScalingCache final {
    public:
        std::unique_ptr<InMemoryMesh> lookupTPSMeshWarp(
            const OpenSim::Model& model,
            const SimTK::State& state,
            const OpenSim::Mesh& inputMesh,
            const OpenSim::Frame& landmarksFrame,
            const ThinPlateSplineCommonInputs& tpsInputs)
        {
            // Compile the TPS coefficients from the source+destination landmarks
            const TPSCoefficients3D<float>& coefficients = lookupTPSCoefficients(tpsInputs);

            // Convert the input mesh into an OSC mesh, so that it's suitable for warping.
            Mesh mesh = ToOscMesh(model, state, inputMesh);

            // Figure out the transform between the input mesh and the landmarks frame
            const auto mesh2landmarks = to<Transform>(inputMesh.getFrame().findTransformBetween(state, landmarksFrame));

            // Warp the verticies in-place.
            auto vertices = mesh.vertices();
            for (auto& vertex : vertices) {
                vertex = transform_point(mesh2landmarks, vertex);  // put vertex into landmark frame
            }
            TPSWarpPointsInPlace(coefficients, vertices, static_cast<float>(tpsInputs.blendingFactor));
            for (auto& vertex : vertices) {
                vertex = inverse_transform_point(mesh2landmarks, vertex);  // put vertex back into mesh frame
            }

            // Assign the vertices back to the OSC mesh and emit it as an `InMemoryMesh` component
            mesh.set_vertices(vertices);
            mesh.recalculate_normals();  // maybe should be a runtime param
            return std::make_unique<InMemoryMesh>(mesh);
        }

        SimTK::Vec3 lookupTPSWarpedRigidPoint(
            [[maybe_unused]] const OpenSim::Model& model,
            const SimTK::State& state,
            const SimTK::Vec3& locationInParent,
            const OpenSim::Frame& parentFrame,
            const OpenSim::Frame& landmarksFrame,
            const ThinPlateSplineCommonInputs& tpsInputs)
        {
            const TPSCoefficients3D<float>& coefficients = lookupTPSCoefficients(tpsInputs);
            const SimTK::Transform stationParentToLandmarksXform = landmarksFrame.getTransformInGround(state).invert() * parentFrame.getTransformInGround(state);
            const SimTK::Vec3 inputLocationInLandmarksFrame = stationParentToLandmarksXform * locationInParent;
            const auto warpedLocationInLandmarksFrame = to<SimTK::Vec3>(TPSWarpPoint(coefficients, to<Vector3>(inputLocationInLandmarksFrame), static_cast<float>(tpsInputs.blendingFactor)));
            const SimTK::Vec3 warpedLocationInStationParentFrame = stationParentToLandmarksXform.invert() * warpedLocationInLandmarksFrame;
            return warpedLocationInStationParentFrame;
        }

        SimTK::Transform lookupTPSAffineTransformWithoutScaling(
            const ThinPlateSplineCommonInputs& tpsInputs)
        {
            OSC_ASSERT_ALWAYS(tpsInputs.applyAffineRotation && "affine rotation must be requested in order to figure out the transform");
            OSC_ASSERT_ALWAYS(tpsInputs.applyAffineTranslation && "affine translation must be requested in order to figure out the transform");
            const TPSCoefficients3D<float>& coefficients = lookupTPSCoefficients(tpsInputs);

            const Vector3d x{normalize(coefficients.a2)};
            const Vector3d y{normalize(coefficients.a3)};
            const Vector3d z{normalize(coefficients.a4)};
            const SimTK::Mat33 rotationMatrix{
                x[0], y[0], z[0],
                x[1], y[1], z[1],
                x[2], y[2], z[2],
            };
            const SimTK::Rotation rotation{rotationMatrix};
            const auto translation = to<SimTK::Vec3>(coefficients.a1);
            return SimTK::Transform{rotation, translation};
        }
    private:

        const TPSCoefficients3D<float>& lookupTPSCoefficients(const ThinPlateSplineCommonInputs& tpsInputs)
        {
            // Read source+destination landmark files into independent collections
            const auto sourceLandmarks = lm::ReadLandmarksFromCSVIntoVectorOrThrow(tpsInputs.sourceLandmarksPath);
            const auto destinationLandmarks = lm::ReadLandmarksFromCSVIntoVectorOrThrow(tpsInputs.destinationLandmarksPath);

            // Pair the source+destination landmarks together into a TPS coefficient solver's inputs
            TPSCoefficientSolverInputs3D<float> inputs;
            inputs.landmarks.reserve(max(sourceLandmarks.size(), destinationLandmarks.size()));
            lm::TryPairingLandmarks(sourceLandmarks, destinationLandmarks, [&inputs, &tpsInputs](const MaybeNamedLandmarkPair& p)
            {
                if (auto landmark3d = p.tryGetPairedLocations()) {
                    landmark3d->source = tpsInputs.sourceLandmarksPrescale * landmark3d->source;
                    landmark3d->destination = tpsInputs.destinationLandmarksPrescale * landmark3d->destination;
                    inputs.landmarks.push_back(*landmark3d);
                }
                else {
                    log_warn("The landmarks %s could not be paired, might be missing in the source/destination?", p.name().c_str());
                }
            });
            inputs.applyAffineTranslation = tpsInputs.applyAffineTranslation;
            inputs.applyAffineScale = tpsInputs.applyAffineScale;
            inputs.applyAffineRotation = tpsInputs.applyAffineRotation;
            inputs.applyNonAffineWarp = tpsInputs.applyNonAffineWarp;

            // Solve the coefficients
            m_CoefficientsTODO = TPSCalcCoefficients(inputs);

            return m_CoefficientsTODO;
        }

        TPSCoefficients3D<float> m_CoefficientsTODO;
    };

    // The state of a validation check performed by a `ScalingStep`.
    enum class ScalingStepValidationState {
        Warning,
        Error,
    };

    // A message produced by a `ScalingStep`'s validation check.
    class ScalingStepValidationMessage {
    public:

        // Constructs a validation message that's related to the value(s) held in
        // a property with name `propertyName` on the `ScalingStep`.
        explicit ScalingStepValidationMessage(
            std::string propertyName,
            ScalingStepValidationState state,
            std::string message) :

            m_MaybePropertyName{std::move(propertyName)},
            m_State{state},
            m_Message{std::move(message)}
        {}

        // Constructs a validation message that's in some (general) way related to
        // the `ScalingStep` that produced it.
        explicit ScalingStepValidationMessage(
            ScalingStepValidationState state,
            std::string message) :

            m_State{state},
            m_Message{std::move(message)}
        {}

        std::optional<CStringView> tryGetPropertyName() const
        {
            return not m_MaybePropertyName.empty() ? std::optional{m_MaybePropertyName} : std::nullopt;
        }
        ScalingStepValidationState getState() const { return m_State; }
        CStringView getMessage() const { return m_Message; }

    private:
        std::string m_MaybePropertyName;
        ScalingStepValidationState m_State;
        std::string m_Message;
    };

    // An abstract base class for a single model-scaling step.
    //
    // Scaling steps are applied one-by-one to a copy of the source model in
    // order to yield the "result" or "scaled" model. Each scaling step can
    // request external data (`ScalingParameterDeclaration`).
    class ScalingStep : public OpenSim::Component {
        OpenSim_DECLARE_ABSTRACT_OBJECT(ScalingStep, Component)

    public:
        OpenSim_DECLARE_PROPERTY(enabled, bool, "toggles applying this scaling step when scaling the model");
        OpenSim_DECLARE_PROPERTY(label, std::string, "a user-facing label for the scaling step");

    protected:
        explicit ScalingStep(std::string_view label)
        {
            constructProperty_enabled(true);
            constructProperty_label(std::string{label});
        }

    public:
        // Returns a user-facing label that describes this `ScalingStep`.
        CStringView label() const { return get_label(); }

        // Sets this `ScalingStep`'s user-facing label.
        void setLabel(CStringView newLabel) { set_label(std::string{newLabel}); }

        // Calls `callback` with each parameter declaration that this `ScalingStep` accepts
        // at scaling-time.
        //
        // It is expected that higher-level engines provide values that satisfy these
        // declarations to `applyScalingStep`.
        void forEachScalingParameterDeclaration(
            const std::function<void(const ScalingParameterDeclaration&)>& callback) const
        {
            implForEachScalingParameterDeclaration(callback);
        }

        // Returns a sequence of `ScalingStepValidationMessage`, which should be empty,
        // or non-errors, before higher-level engines call `applyScalingStep` (otherwise,
        // an exception may be thrown by `applyScalingStep`).
        std::vector<ScalingStepValidationMessage> validate(
            ScalingCache& scalingCache,
            const ScalingParameters& scalingParameters,
            const OpenSim::Model& sourceModel) const
        {
            return implValidate(scalingCache, scalingParameters, sourceModel);
        }

        // Applies this `ScalingStep`'s scaling function in-place to the `resultModel`. The
        // original `sourceModel` is also provided, if relevant.
        //
        // It is expected that `scalingParameters` contains at least the scaling parameter
        // values that match the declarations emitted by `forEachScalingParameterDeclaration`.
        void applyScalingStep(
            ScalingCache& scalingCache,
            const ScalingParameters& scalingParameters,
            const OpenSim::Model& sourceModel,
            OpenSim::Model& resultModel) const
        {
            if (get_enabled()) {
                implApplyScalingStep(scalingCache, scalingParameters, sourceModel, resultModel);
            }
        }
    private:
        // Implementors should provide the callback with any `ScalingParameterDeclaration`s in order
        // to ensure that the runtime can later provide the `ScalingParameterValue` during model
        // scaling.
        virtual void implForEachScalingParameterDeclaration(const std::function<void(const ScalingParameterDeclaration&)>&) const
        {}

        // Implementors should return any validation warnings/errors related to this scaling step
        // (e.g. incorrect property value, missing external data, etc.).
        virtual std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model&) const
        {
            return {};  // i.e. by default, return no validation errors.
        }

        // Implementors should apply their scaling to the result model (the source model is also
        // available). Any computationally expensive scaling steps should be performed via
        // the `ScalingCache`.
        virtual void implApplyScalingStep(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model&,
            OpenSim::Model&) const
        {}
    };

    // Common parameters that can be requested from the `ThinPlateSplineScalingStep` base class.
    struct ThinPlateSplineScalingStepCommonParams final {
        ThinPlateSplineCommonInputs tpsInputs;
        const OpenSim::Frame* landmarksFrame = nullptr;
    };

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
        OpenSim_DECLARE_PROPERTY(source_landmarks_prescale, double, "Scaling factor that each source landmark point should be multiplied by before computing the TPS warp. This is sometimes necessary if (e.g.) the mesh is in different units (OpenSim works in meters).");
        OpenSim_DECLARE_PROPERTY(destination_landmarks_prescale, double, "Scaling factor that each destination landmark point should be multiplied by before computing the TPS warp. This is sometimes necessary if (e.g.) the mesh is in different units (OpenSim works in meters).");

    protected:

        // Constructs a `ThinPlateSplineScalingStep` with defaulted parameters.
        explicit ThinPlateSplineScalingStep(std::string_view label) :
            ScalingStep{label}
        {
            constructProperty_source_landmarks_file({});
            constructProperty_destination_landmarks_file({});
            constructProperty_landmarks_frame("/ground");
            constructProperty_source_landmarks_prescale(1.0);
            constructProperty_destination_landmarks_prescale(1.0);
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
            const auto modelFilesystemLocation = TryFindInputFile(sourceModel);
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
        ThinPlateSplineScalingStepCommonParams calcTPSScalingStepCommonParams(
            const ScalingParameters& parameters,
            OpenSim::Model& resultModel) const
        {
            // Lookup/validate warping inputs.
            const std::optional<std::filesystem::path> modelFilesystemLocation = TryFindInputFile(resultModel);
            OSC_ASSERT_ALWAYS(modelFilesystemLocation && "The source model has no filesystem location");

            OSC_ASSERT_ALWAYS(not get_source_landmarks_file().empty());
            const std::filesystem::path sourceLandmarksPath = modelFilesystemLocation->parent_path() / get_source_landmarks_file();

            OSC_ASSERT_ALWAYS(not get_destination_landmarks_file().empty());
            const std::filesystem::path destinationLandmarksPath = modelFilesystemLocation->parent_path() / get_destination_landmarks_file();

            OSC_ASSERT_ALWAYS(not get_landmarks_frame().empty());
            const auto* landmarksFrame = FindComponent<OpenSim::Frame>(resultModel, get_landmarks_frame());
            OSC_ASSERT_ALWAYS(landmarksFrame && "could not find the landmarks frame in the model");

            const std::optional<double> blendingFactor = parameters.lookup<double>("blending_factor");
            OSC_ASSERT_ALWAYS(blendingFactor && "blending_factor was not set by the warping engine");

            return ThinPlateSplineScalingStepCommonParams{
                .tpsInputs = ThinPlateSplineCommonInputs{
                    sourceLandmarksPath,
                    destinationLandmarksPath,
                    get_source_landmarks_prescale(),
                    get_destination_landmarks_prescale(),
                    *blendingFactor,
                },
                .landmarksFrame = landmarksFrame,
            };
        }
    };

    // An abstract base class for a `ThinPlateSplineScalingStep` that has user-editable
    // toggles for parts of the TPS algorithm.
    class ToggleableThinPlateSplineScalingStep : public ThinPlateSplineScalingStep {
        OpenSim_DECLARE_ABSTRACT_OBJECT(ToggleableThinPlateSplineScalingStep, ThinPlateSplineScalingStep)
    public:
        OpenSim_DECLARE_PROPERTY(apply_affine_translation, bool, "Enable/disable applying the affine translational part of the TPS warp to the output. This can be useful/necessary if/when it's handled by some other part of the model (e.g. an offset frame, joint frame).");
        OpenSim_DECLARE_PROPERTY(apply_affine_scale, bool, "Enable/disable applying the affine scaling part of the TPS warp to the output. This can be useful/necessary for visually inspecting the difference between the non-affine scaling components and the affine parts.");
        OpenSim_DECLARE_PROPERTY(apply_affine_rotation, bool, "Enable/disable applying the application of the affine rotational part of the TPS warp to the output. This can be useful/necessary if/when it's handled by some other part of the model (e.g. an offset frame, joint frame).");
        OpenSim_DECLARE_PROPERTY(apply_non_affine_warp, bool, "Enable/disable applying the non-affine (i.e. the 'warp-ey part') of the TPS warp to the output.");

    protected:
        explicit ToggleableThinPlateSplineScalingStep(std::string_view label) :
            ThinPlateSplineScalingStep{label}
        {
            constructProperty_apply_affine_translation(false);
            constructProperty_apply_affine_scale(true);
            constructProperty_apply_affine_rotation(false);
            constructProperty_apply_non_affine_warp(true);
        }

        ThinPlateSplineScalingStepCommonParams calcTPSScalingStepCommonParams(
            const ScalingParameters& parameters,
            OpenSim::Model& resultModel) const
        {
            auto rv = ThinPlateSplineScalingStep::calcTPSScalingStepCommonParams(parameters, resultModel);
            rv.tpsInputs.applyAffineTranslation = get_apply_affine_translation();
            rv.tpsInputs.applyAffineScale = get_apply_affine_scale();
            rv.tpsInputs.applyAffineRotation = get_apply_affine_rotation();
            rv.tpsInputs.applyNonAffineWarp = get_apply_non_affine_warp();
            return rv;
        }
    };

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
                    std::stringstream msg;
                    msg << get_meshes(i) << ": Cannot find entry in 'meshes' in the source model (or it isn't a Mesh).";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
                }
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache& scalingCache,
            const ScalingParameters& parameters,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, resultModel);

            // Warp each mesh specified by the `meshes` property.
            for (int i = 0; i < getProperty_meshes().size(); ++i) {
                // Find the input mesh and use it produce the warped mesh.
                const auto* mesh = FindComponent<OpenSim::Mesh>(resultModel, get_meshes(i));
                OSC_ASSERT_ALWAYS(mesh && "could not find a mesh in the source model");
                std::unique_ptr<InMemoryMesh> warpedMesh = scalingCache.lookupTPSMeshWarp(
                    resultModel,
                    resultModel.getWorkingState(),
                    *mesh,
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );
                OSC_ASSERT_ALWAYS(warpedMesh && "warping a mesh in the model failed");

                // Overwrite the mesh in the result model with the warped mesh.
                auto* resultMesh = FindComponentMut<OpenSim::Mesh>(resultModel, get_meshes(i));
                OSC_ASSERT_ALWAYS(resultMesh && "could not find a corresponding mesh in the result model");
                OverwriteGeometry(resultModel, *resultMesh, std::move(warpedMesh));
                OSC_ASSERT_ALWAYS(FindComponent<InMemoryMesh>(resultModel, get_meshes(i)) != nullptr);
            }
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };

    // A `ScalingStep` that applies the Thin-Plate Spline (TPS) warp to any
    // `OpenSim::Station`s it can find via the `stations` search string.
    class ThinPlateSplineStationsScalingStep final : public ToggleableThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineStationsScalingStep, ThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(stations, std::string, "Query paths (e.g. `/forceset/*`) that the engine should use to find stations in the source model that should be warped by this scaling step.");

    public:
        explicit ThinPlateSplineStationsScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to Stations"}
        {
            setDescription("Warps the locations of stations in the model using the Thin-Plate Spline (TPS) warping algorithm.");
            constructProperty_stations();
        }
    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache& cache,
            const ScalingParameters& params,
            const OpenSim::Model& sourceModel) const final
        {
            // Get base class validation messages.
            auto messages = ToggleableThinPlateSplineScalingStep::implValidate(cache, params, sourceModel);

            // Ensure every entry in `stations` can be found in the source model.
            for (int i = 0; i < getProperty_stations().size(); ++i) {
                const auto* station = FindComponent<OpenSim::Station>(sourceModel, get_stations(i));
                if (not station) {
                    std::stringstream msg;
                    msg << get_stations(i) << ": Cannot a Station in 'stations' in the source model (or it isn't a Station).";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
                }
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache& scalingCache,
            const ScalingParameters& parameters,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, resultModel);

            // Warp each station specified by the `stations` property.
            for (int i = 0; i < getProperty_stations().size(); ++i) {
                // Find the input station and use it produce the warped station.
                const auto* station = FindComponent<OpenSim::Station>(resultModel, get_stations(i));
                OSC_ASSERT_ALWAYS(station && "could not find a station in the source model");

                const SimTK::Vec3 warpedLocation = scalingCache.lookupTPSWarpedRigidPoint(
                    resultModel,
                    resultModel.getWorkingState(),
                    station->get_location(),
                    station->getParentFrame(),
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );

                auto* resultStation = FindComponentMut<OpenSim::Station>(resultModel, get_stations(i));
                OSC_ASSERT_ALWAYS(resultStation && "could not find a corresponding station in the result model");
                resultStation->set_location(warpedLocation);
            }
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };

    // A `ScalingStep` that applies the Thin-Plate Spline (TPS) warp to any
    // `OpenSim::PathPoint`s it can find via the `path_points` search string.
    class ThinPlateSplinePathPointsScalingStep final : public ToggleableThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplinePathPointsScalingStep, ToggleableThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(path_points, std::string, "Query paths (e.g. `/forceset/*`) that the engine should use to find path points in the source model that should be warped by this scaling step.");

    public:
        explicit ThinPlateSplinePathPointsScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to Path Points"}
        {
            setDescription("Warps the locations of path points in the model using the Thin-Plate Spline (TPS) warping algorithm.");
            constructProperty_path_points();
        }
    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache& cache,
            const ScalingParameters& params,
            const OpenSim::Model& sourceModel) const final
        {
            // Get base class validation messages.
            auto messages = ToggleableThinPlateSplineScalingStep::implValidate(cache, params, sourceModel);

            // Ensure every entry in `path_points` can be found in the source model.
            for (int i = 0; i < getProperty_path_points().size(); ++i) {
                const auto* pathPoint = FindComponent<OpenSim::PathPoint>(sourceModel, get_path_points(i));
                if (not pathPoint) {
                    std::stringstream msg;
                    msg << get_path_points(i) << ": Cannot find a PathPoint in 'path_points' in the source model (or it isn't a PathPoint)";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
                }
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache& scalingCache,
            const ScalingParameters& parameters,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, resultModel);

            // Warp each path point specified by the `path_points` property.
            for (int i = 0; i < getProperty_path_points().size(); ++i) {
                // Find the path point in the source model and use it produce the warped path point.
                const auto* pathPoint = FindComponent<OpenSim::PathPoint>(resultModel, get_path_points(i));
                OSC_ASSERT_ALWAYS(pathPoint && "could not find a path point in the source model");

                const SimTK::Vec3 warpedLocation = scalingCache.lookupTPSWarpedRigidPoint(
                    resultModel,
                    resultModel.getWorkingState(),
                    pathPoint->get_location(),
                    pathPoint->getParentFrame(),
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );

                auto* resultPathPoint = FindComponentMut<OpenSim::PathPoint>(resultModel, get_path_points(i));
                OSC_ASSERT_ALWAYS(resultPathPoint && "could not find a corresponding path point in the result model");
                resultPathPoint->set_location(warpedLocation);
            }
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };

    // A `ScalingStep` that applies the Thin-Plate Spline (TPS) warp to the
    // `translation` property of an `OpenSim::PhysicalOffsetFrame`.
    class ThinPlateSplineOffsetFrameTranslationScalingStep final : public ToggleableThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineOffsetFrameTranslationScalingStep, ToggleableThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(offset_frames, std::string, "Absolute paths (e.g. `/jointset/joint/parent_frame`) that the engine should use to find the offset frames in the source model.");

    public:
        explicit ThinPlateSplineOffsetFrameTranslationScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to Offset Frame translation"}
        {
            setDescription("Uses the Thin-Plate Spline (TPS) warping algorithm to warp the translation of the given offset frames.");
            constructProperty_offset_frames();
        }

    private:

        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache& cache,
            const ScalingParameters& params,
            const OpenSim::Model& sourceModel) const final
        {
            // Get base class validation messages.
            auto messages = ToggleableThinPlateSplineScalingStep::implValidate(cache, params, sourceModel);

            // Ensure every entry in `offset_frames` can be found in the source model.
            for (int i = 0; i < getProperty_offset_frames().size(); ++i) {
                const auto* offsetFrame = FindComponent<OpenSim::PhysicalOffsetFrame>(sourceModel, get_offset_frames(i));
                if (not offsetFrame) {
                    std::stringstream msg;
                    msg << get_offset_frames(i) << ": Cannot find a `PhysicalOffsetFrame` in 'offset_frames' in the source model (or it isn't a `PhysicalOffsetFrame`).";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
                }
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache& scalingCache,
            const ScalingParameters& parameters,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            // Lookup/validate warping inputs.
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, resultModel);

            // Warp each offset frame `translation` specified by the `offset_frames` property.
            for (int i = 0; i < getProperty_offset_frames().size(); ++i) {
                // Find the path point in the source model and use it produce the warped path point.
                const auto* offsetFrame = FindComponent<OpenSim::PhysicalOffsetFrame>(resultModel, get_offset_frames(i));
                OSC_ASSERT_ALWAYS(offsetFrame && "could not find a `PhysicalOffsetFrame` in the source model");

                const SimTK::Vec3 warpedLocation = scalingCache.lookupTPSWarpedRigidPoint(
                    resultModel,
                    resultModel.getWorkingState(),
                    offsetFrame->get_translation(),
                    offsetFrame->getParentFrame(),
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );

                auto* resultOffsetFrame = FindComponentMut<OpenSim::PhysicalOffsetFrame>(resultModel, get_offset_frames(i));
                OSC_ASSERT_ALWAYS(resultOffsetFrame && "could not find a corresponding `PhysicalOffsetFrame` in the result model");
                resultOffsetFrame->set_translation(warpedLocation);
            }
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };

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

                std::stringstream msg;
                msg << destinationMeshPath.string() << ": Cannot find `destination_mesh_file` on filesystem";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            // Ensure `source_mesh_component_path` exists in the model
            const auto* sourceMesh = FindComponent<OpenSim::Mesh>(sourceModel, get_source_mesh_component_path());
            if (not sourceMesh) {
                std::stringstream msg;
                msg << get_landmarks_frame() << ": Cannot find Mesh 'source_mesh_component_path' in the source model (or it isn't a Mesh).";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache& cache,
            const ScalingParameters& parameters,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            // Lookup/validate warping inputs.
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, resultModel);

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

    // A `ThinPlateSpline` scaling step that tries to scale the origin, orientation, radius,
    // length, and quadrant of the given `WrapCylinder`s.
    class ThinPlateSplineWrapCylinderScalingStep final : public ToggleableThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineWrapCylinderScalingStep, ThinPlateSplineScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(wrap_cylinders, std::string, "Absolute paths (e.g. `/bodyset/body/wrap_cylinder_2`) to `WrapCylinder` components in the model");
        OpenSim_DECLARE_PROPERTY(midline_projection_distance, double, "The distance, in meters, that the `WrapCylinder`s' midlines should be projected from each of their origin points before putting them through the TPS algorithm. This is used to figure out the new orientation of the `WrapCylinder`s.");
        OpenSim_DECLARE_PROPERTY(surface_projection_theta, double, "An angle, in radians, of a vector that originates in each `WrapCylinder`'s origin, spins around their Z axis with an angle of theta from the X axis, and has a length of each `WrapCylinder`'s `radius`, producing a surface point. Each surface point is fed through the TPS algorithm and used to recalculate each `WrapCylinder`'s `radius` and `quadrant`");
    public:
        explicit ThinPlateSplineWrapCylinderScalingStep() :
            ToggleableThinPlateSplineScalingStep{"Apply Thin-Plate Spline (TPS) to WrapCylinder"}
        {
            setDescription(R"(
Uses the Thin-Plate Spline (TPS) warping algorithm to scale `WrapCylinder`s in the model:

  - Warps the `translation` of each `WrapCylinder` by warping the source `translation` with
    the TPS algorithm to produce a new `translation`.
  - Warps the `xyz_body_rotation` of each `WrapCylinder` by projecting a point from its
    source origin (`translation`) `midline_projection_distance` along the cylinder's
    midline (+Z), warping it with the TPS algorithm, and then calculating a new
    `xyz_body_orientation` that orients the `WrapCylinder` such that the vector between the
    new origin (the new `translation`) and the projected point becomes the new
    `WrapCylinder` midline. To prevent the cylinder from spinning along this vector, the
    along-axis rotation is constrained by the location of a point produced by
    `surface_projection_theta`.
  - Warps the `radius` of each `WrapCylinder` by calculating a point on each of their surfaces
    specified `surface_projection_theta`. The points are then warped with the TPS algorithm to
    produce (presumed to be) new surface points. The `radius` of each `WrapCylinder` is equal
    to the distance between their respective midlines and projected surface points.
  - Does not warp `quadrant`. It is assumed that constraining the calculation of `xyz_body_rotation`
    will cause the warped cylinder to have a like-for-like orientation with respect to whatever's
    being wrapped.
  - Does not warp `length`. There is (currently) no easy way to do this, because the ends of
    `WrapCylinder`s tend to be far away from where the TPS warp is defined, which makes it hard
    to warp+project the endcaps. It is recommended that `WrapCylinder`s are long enough to ensure
    they are not sensitive to model scaling (very few models require muscles to fall of the end
    of the cylinder).
)");
            constructProperty_wrap_cylinders();
            constructProperty_midline_projection_distance(0.001);  // 1 mm
            constructProperty_surface_projection_theta(0.0);
        }
    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache& cache,
            const ScalingParameters& params,
            const OpenSim::Model& sourceModel) const final
        {
            // Get base class validation messages.
            auto messages = ToggleableThinPlateSplineScalingStep::implValidate(cache, params, sourceModel);

            // Ensure every entry in `wrap_cylinders` can be found in the source model.
            for (int i = 0; i < getProperty_wrap_cylinders().size(); ++i) {
                const auto* offsetFrame = FindComponent<OpenSim::WrapCylinder>(sourceModel, get_wrap_cylinders(i));
                if (not offsetFrame) {
                    std::stringstream msg;
                    msg << get_wrap_cylinders(i) << ": Cannot find a `WrapCylinder` in 'wrap_cylinders' in the source model (or it isn't a `WrapCylinder`).";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
                }
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache& cache,
            const ScalingParameters& parameters,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            // Lookup/validate warping inputs.
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, resultModel);

            // Precalculate the surface point direction vector as "rotate a unit vector pointing
            // along X around the Z axis by theta amount" (see property documentation).
            const SimTK::Rotation rotateCylinderXToSurfacePointDirection{get_surface_projection_theta(), SimTK::ZAxis};
            const SimTK::UnitVec3 surfacePointDirectionInCylinderSpace{rotateCylinderXToSurfacePointDirection * SimTK::Vec3{1.0, 0.0, 0.0}};

            // Precalculate the midline projection point in the cylinder's local (not parent) space.
            const SimTK::Vec3 sourceMidlinePointInCylinderSpace{0.0, 0.0, get_midline_projection_distance()};

            // Warp each `WrapCylinder` specified by the `wrap_cylinders` property.
            for (int i = 0; i < getProperty_wrap_cylinders().size(); ++i) {
                // Find the `i`th `WrapCylinder` in the model.
                auto* wrapCylinder = FindComponentMut<OpenSim::WrapCylinder>(resultModel, get_wrap_cylinders(i));
                OSC_ASSERT_ALWAYS(wrapCylinder && "could not find a `WrapCylinder` in the source model");

                // Calculate the `WrapCylinder`'s new `translation` by warping the origin.
                const SimTK::Vec3 newOriginPointInParent = cache.lookupTPSWarpedRigidPoint(
                    resultModel,
                    resultModel.getWorkingState(),
                    wrapCylinder->get_translation(),
                    wrapCylinder->getFrame(),
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );

                // Calculate the `WrapCylinder`'s new projected midline point by warping it.
                const SimTK::Vec3 sourceMidlinePointInParent = wrapCylinder->getTransform() * sourceMidlinePointInCylinderSpace;
                const SimTK::Vec3 newMidlinePointInParent = cache.lookupTPSWarpedRigidPoint(
                    resultModel,
                    resultModel.getWorkingState(),
                    sourceMidlinePointInParent,
                    wrapCylinder->getFrame(),
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );

                // Calculate the source surface point by projecting the direction onto the `WrapCylinder`'s surface.
                const SimTK::Vec3 sourceSurfacePointInCylinderSpace = wrapCylinder->get_radius() * surfacePointDirectionInCylinderSpace;
                const SimTK::Vec3 sourceSurfacePointInParent = wrapCylinder->getTransform() * sourceSurfacePointInCylinderSpace;
                const SimTK::Vec3 newSurfacePointInParent = cache.lookupTPSWarpedRigidPoint(
                    resultModel,
                    resultModel.getWorkingState(),
                    sourceSurfacePointInParent,
                    wrapCylinder->getFrame(),
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );

                // The `WrapCylinder`'s new Z axis within the parent frame is a unit vector that
                // points from its new origin to the projected midline point.
                const SimTK::UnitVec3 newZAxisDirectionInParent{newMidlinePointInParent - newOriginPointInParent};

                // We can now use the warped surface point to figure out what the orientation _around_
                // Z should be (i.e. to constrain it, rather than letting the orientation spin along
                // the Z axis).
                //
                // Due to the non-linearity of TPS, the warped surface point may require
                // reorthogonalization with respect to the `WrapCylinder`'s new midline. Here, we use
                // vector rejection (the opposite of vector projection) to compute an orthogonal vector
                // that we can un-spin by `surface_projection_theta` in order to figure out exactly where
                // the X axis should point.
                const SimTK::UnitVec3 newSurfacePointDirectionInParent{newSurfacePointInParent - newOriginPointInParent};
                OSC_ASSERT((SimTK::dot(newZAxisDirectionInParent, newSurfacePointDirectionInParent) < 1.0 - SimTK::Eps) && "cylinder surface point somehow points along Z axis - warping is too strong?");
                const SimTK::Vec3 newSurfacePointProjectionAlongZVector = newZAxisDirectionInParent * SimTK::dot(newZAxisDirectionInParent, newSurfacePointDirectionInParent);
                const SimTK::Vec3 newSurfacePointRejectionVector = newSurfacePointDirectionInParent - newSurfacePointProjectionAlongZVector;
                const SimTK::UnitVec3 newSurfacePointRejectionDirection{newSurfacePointRejectionVector};

                // The new surface point direction is assumed to be `surface_projection_theta` rotated along
                // the new Z axis from where the X axis should be (that's how it was initially generated), so
                // un-rotate it to figure out where the new X axis should be.
                const SimTK::UnitVec3 newXAxisDirectionInParent{rotateCylinderXToSurfacePointDirection.invert() * newSurfacePointRejectionDirection};

                // With two vectors pointing along known axes, we can compute a new cylinder rotation
                const SimTK::Rotation newCylinderRotation{newZAxisDirectionInParent, SimTK::ZAxis, newXAxisDirectionInParent, SimTK::XAxis};

                // The new radius is the projection of the surface point onto the X axis
                const double newRadius = SimTK::dot(SimTK::Vec3{newSurfacePointInParent - newOriginPointInParent}, newXAxisDirectionInParent);

                // (finally) Update the cylinder
                wrapCylinder->set_translation(newOriginPointInParent);
                wrapCylinder->set_xyz_body_rotation(newCylinderRotation.convertRotationToBodyFixedXYZ());
                wrapCylinder->set_radius(newRadius);
            }
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };

    // A `ScalingStep` that scales the masses of bodies in the model.
    class ModelMassToSubjectMassScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ModelMassToSubjectMassScalingStep, ScalingStep)
    public:
        explicit ModelMassToSubjectMassScalingStep() :
            ScalingStep{"Scale Model Mass to Subject Mass"}
        {
            setDescription("Scales the masses of bodies in the model to match the subject mass, while preserving the overall mass disribution of each body in the model.");
        }

    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache&,
            const ScalingParameters& parameters,
            const OpenSim::Model& model) const final
        {
            std::vector<ScalingStepValidationMessage> messages;

            if (parameters.lookup<double>("subject_mass").value_or(0.0) <= 0.0) {
                std::stringstream msg;
                msg << "The subject_mass scaling parameter must be greater than zero";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            if (model.getTotalMass(model.getWorkingState()) <= 0.0) {
                std::stringstream msg;
                msg << "Cannot scale the model's mass to the subject's mass because the model itself has a mass of zero";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            return messages;
        }

        void implForEachScalingParameterDeclaration(const std::function<void(const ScalingParameterDeclaration&)>& callback) const final
        {
            callback(ScalingParameterDeclaration{"blending_factor", 1.0});
            callback(ScalingParameterDeclaration{"subject_mass", 75.0});
        }

        void implApplyScalingStep(
            ScalingCache&,
            const ScalingParameters& parameters,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            const auto subjectMass = parameters.lookup<double>("subject_mass").value_or(0.0);
            OSC_ASSERT_ALWAYS(subjectMass > 0.0 && "Subject mass must be greater than zero");

            const std::optional<double> blendingFactor = parameters.lookup<double>("blending_factor");
            OSC_ASSERT_ALWAYS(blendingFactor && "blending_factor was not set by the warping engine");

            const auto effectiveTargetMass = lerp(resultModel.getTotalMass(resultModel.getWorkingState()), subjectMass, *blendingFactor);

            ScaleModelMassPreserveMassDistribution(resultModel, resultModel.getWorkingState(), effectiveTargetMass);
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };

    // A `ScalingStep` that scales body segments by the chosen (potentially, non-uniform) scale factors
    class ManuallyScaleBodySegmentsScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ManuallyScaleBodySegmentsScalingStep, ScalingStep)

        OpenSim_DECLARE_LIST_PROPERTY(bodies, std::string, "Absolute paths (e.g. `/bodyset/femur`) to `Body` components in the model");
        OpenSim_DECLARE_PROPERTY(scale_factors, SimTK::Vec3, "The manual scale factors that should be applied to the specified bodies");
        OpenSim_DECLARE_PROPERTY(preserve_masses, bool, "If enabled, the masses of the bodies will not be scaled (inertial properties will still be scaled)");
    public:
        explicit ManuallyScaleBodySegmentsScalingStep() :
            ScalingStep{"Manually Scale Body Segments"}
        {
            setDescription("Applies a manually-specified scaling factor to the given body segments in the model.");
            constructProperty_bodies();
            constructProperty_scale_factors(SimTK::Vec3{1.0});
            constructProperty_preserve_masses(false);
        }

    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache&,
            const ScalingParameters& parameters,
            const OpenSim::Model& sourceModel) const final
        {
            std::vector<ScalingStepValidationMessage> messages;

            // Ensure every entry in `wrap_cylinders` can be found in the source model.
            for (int i = 0; i < getProperty_bodies().size(); ++i) {
                const auto* body = FindComponent<OpenSim::Body>(sourceModel, get_bodies(i));
                if (not body) {
                    std::stringstream msg;
                    msg << get_bodies(i) << ": Cannot find a `Body` in 'bodies' in the source model (or it isn't a `Body`).";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
                }
            }

            const std::optional<double> blendingFactor = parameters.lookup<double>("blending_factor");
            OSC_ASSERT_ALWAYS(blendingFactor && "blending_factor was not set by the warping engine");

            return messages;
        }

        void implForEachScalingParameterDeclaration(const std::function<void(const ScalingParameterDeclaration&)>& callback) const final
        {
            callback(ScalingParameterDeclaration{"blending_factor", 1.0});
        }

        void implApplyScalingStep(
            ScalingCache&,
            const ScalingParameters& parameters,
            const OpenSim::Model&,
            OpenSim::Model& model) const final
        {
            const std::optional<double> blendingFactor = parameters.lookup<double>("blending_factor");
            OSC_ASSERT_ALWAYS(blendingFactor && "blending_factor was not set by the warping engine");

            const SimTK::Vec3 scaleFactors = {
                lerp(1.0, get_scale_factors()[0], *blendingFactor),
                lerp(1.0, get_scale_factors()[1], *blendingFactor),
                lerp(1.0, get_scale_factors()[2], *blendingFactor),
            };

            OpenSim::ScaleSet scaleSet;
            for (int i = 0; i < getProperty_bodies().size(); ++i) {
                const auto* body = FindComponent<OpenSim::Body>(model, get_bodies(i));
                if (not body) {
                    std::stringstream msg;
                    msg << get_bodies(i) << ": Cannot find a `Body` in 'bodies' in the source model (or it isn't a `Body`).";
                    throw std::runtime_error{std::move(msg).str()};
                }

                auto scale = std::make_unique<OpenSim::Scale>();
                scale->setSegmentName(body->getName());
                scale->setScaleFactors(scaleFactors);
                scale->setApply(true);
                scaleSet.adoptAndAppend(scale.release());
            }
            model.scale(model.updWorkingState(), scaleSet, get_preserve_masses());
            InitializeModel(model);
            InitializeState(model);
        }
    };

    class RecalculateWrapCylinderRadiusFromStationScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(RecalculateWrapCylinderRadiusFromStationScalingStep, ScalingStep)
    public:
        OpenSim_DECLARE_PROPERTY(station_path, std::string, "Absolute path (e.g. `/componentset/some_station`) to a `Station` component in the model");
        OpenSim_DECLARE_PROPERTY(wrap_cylinder_path, std::string, "Absolute path (e.g. `/bodyset/body/wrap_cylinder_2`) to a `WrapCylinder` component in the model");

        explicit RecalculateWrapCylinderRadiusFromStationScalingStep() :
            ScalingStep{"Recalculate WrapCylinder `radius` from Station Projection onto its Midline"}
        {
            setDescription("Recalculates the 'radius' of a `WrapCylinder` component, located at `wrap_cylinder_path`, as the distance between the `Station`, located at `station_path`, and the cylinder's (infinitely long) midline.");
            constructProperty_station_path("");
            constructProperty_wrap_cylinder_path("");
        }
    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model& model) const final
        {
            std::vector<ScalingStepValidationMessage> messages;

            // Ensure `station_path` exists in the model (and is a `Station`)
            const auto* station = FindComponent<OpenSim::Station>(model, get_station_path());
            if (not station) {
                std::stringstream msg;
                msg << get_station_path() << ": Cannot find `station_path` in the source model (or it isn't a Station).";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            // Ensure `wrap_cylinder_path` exists in the model (and is a `WrapCylinder`)
            const auto* wrapCylinder = FindComponent<OpenSim::WrapCylinder>(model, get_wrap_cylinder_path());
            if (not wrapCylinder) {
                std::stringstream msg;
                msg << get_wrap_cylinder_path() << ": Cannot find 'wrap_cylinder_path' in the source model (or it isn't a `WrapCylinder`).";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            const auto* station = FindComponent<OpenSim::Station>(resultModel, get_station_path());
            OSC_ASSERT_ALWAYS(station && "could not find a station in the model");
            auto* wrapCylinder = FindComponentMut<OpenSim::WrapCylinder>(resultModel, get_wrap_cylinder_path());
            OSC_ASSERT_ALWAYS(wrapCylinder && "could not find a wrap cylinder in the model");

            // Put the station into the cylinder's reference frame
            const SimTK::Transform cylinder2cylinderFrame = wrapCylinder->getTransform();
            const SimTK::Transform cylinderFrame2ground = wrapCylinder->getFrame().getTransformInGround(resultModel.getWorkingState());
            const SimTK::Transform ground2cylinder = (cylinderFrame2ground * cylinder2cylinderFrame).invert();
            const SimTK::Vec3 pCylinder = ground2cylinder * station->getLocationInGround(resultModel.getWorkingState());

            // In the cylinder's frame, Z (from origin) is the centerline of the cylinder, so the easiest way to
            // figure out the new radius is just to compute the XY norm from the origin and ignore Z.
            const auto newRadius = SimTK::Vec2{pCylinder[0], pCylinder[1]}.norm();

            // Update accordingly
            wrapCylinder->set_radius(newRadius);
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };

    class RecalculateWrapCylinderXYZBodyRotationFromStationScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(RecalculateWrapCylinderXYZBodyRotationFromStationScalingStep, ScalingStep)
    public:
        OpenSim_DECLARE_PROPERTY(station_path, std::string, "Absolute path (e.g. `/componentset/some_station`) to a `Station` component in the model");
        OpenSim_DECLARE_PROPERTY(wrap_cylinder_path, std::string, "Absolute path (e.g. `/bodyset/body/wrap_cylinder_2`) to a `WrapCylinder` component in the model");

        explicit RecalculateWrapCylinderXYZBodyRotationFromStationScalingStep() :
            ScalingStep{"Recalculate WrapCylinder 'xyz_body_rotation' from Station Placed Along Its Midline"}
        {
            setDescription("Recalculates the 'xyz_body_rotation' of a `WrapCylinder` component, located at `wrap_cylinder_path` such that the cylinder's +Z direction (midline) points toward the Station component located at `station_path`");
            constructProperty_station_path("");
            constructProperty_wrap_cylinder_path("");
        }
    private:
        std::vector<ScalingStepValidationMessage> implValidate(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model& model) const final
        {
            std::vector<ScalingStepValidationMessage> messages;

            // Ensure `station_path` exists in the model (and is a `Station`)
            const auto* station = FindComponent<OpenSim::Station>(model, get_station_path());
            if (not station) {
                std::stringstream msg;
                msg << get_station_path() << ": Cannot find `station_path` in the source model (or it isn't a Station).";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            // Ensure `wrap_cylinder_path` exists in the model (and is a `WrapCylinder`)
            const auto* wrapCylinder = FindComponent<OpenSim::WrapCylinder>(model, get_wrap_cylinder_path());
            if (not wrapCylinder) {
                std::stringstream msg;
                msg << get_wrap_cylinder_path() << ": Cannot find 'wrap_cylinder_path' in the source model (or it isn't a `WrapCylinder`).";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            return messages;
        }

        void implApplyScalingStep(
            ScalingCache&,
            const ScalingParameters&,
            const OpenSim::Model&,
            OpenSim::Model& resultModel) const final
        {
            using std::acos;
            using std::abs;

            const auto* station = FindComponent<OpenSim::Station>(resultModel, get_station_path());
            OSC_ASSERT_ALWAYS(station && "could not find a station in the model");
            auto* wrapCylinder = FindComponentMut<OpenSim::WrapCylinder>(resultModel, get_wrap_cylinder_path());
            OSC_ASSERT_ALWAYS(wrapCylinder && "could not find a wrap cylinder in the model");

            // Re-express the station in the cylinder's reference frame
            const SimTK::Transform cylinder2cylinderFrame = wrapCylinder->getTransform();
            const SimTK::Transform cylinderFrame2ground = wrapCylinder->getFrame().getTransformInGround(resultModel.getWorkingState());
            const SimTK::Transform ground2cylinder = (cylinderFrame2ground * cylinder2cylinderFrame).invert();
            const SimTK::Vec3 stationPosInCylinder = ground2cylinder * station->getLocationInGround(resultModel.getWorkingState());

            // Calculate the source/target cylinder midline directions.
            const SimTK::UnitVec3 cylinderDirectionInCylinder{SimTK::CoordinateAxis::ZCoordinateAxis{}};
            const SimTK::UnitVec3 stationDirectionInCylinder{stationPosInCylinder};

            // Edge-case: If the station is pointing along the Z axis, no reorientation is necessary
            const auto cosRotationAngle = SimTK::dot(cylinderDirectionInCylinder, stationDirectionInCylinder);
            if (abs(cosRotationAngle) >= 1.0 - SimTK::Eps) {
                return;  // Nothing to do
            }

            // Else: If the station isn't pointing along the Z axis (usual case), calculate a rotation that
            //       rotates the cylinder's coordinate space so that +Z points toward toward the station.
            const auto rotationAngle = acos(cosRotationAngle);
            const SimTK::UnitVec3 rotationAxis{SimTK::cross(cylinderDirectionInCylinder, stationDirectionInCylinder)};
            const SimTK::Rotation rotation{rotationAngle, rotationAxis};

            // Compose the additional rotation with the original one to calculate the necessary overall rotation
            const SimTK::Rotation newRotation = cylinder2cylinderFrame.R() * rotation;

            // Write the new rotation to the `WrapCylinder`'s `xyz_body_rotation` property as Euler angles
            wrapCylinder->set_xyz_body_rotation(newRotation.convertRotationToBodyFixedXYZ());
            InitializeModel(resultModel);
            InitializeState(resultModel);
        }
    };

    // Compile-time `Typelist` containing all `ScalingStep`s that this UI handles.
    using AllScalingStepTypes = Typelist<
        ThinPlateSplineMeshesScalingStep,
        ThinPlateSplineStationsScalingStep,
        ThinPlateSplinePathPointsScalingStep,
        ThinPlateSplineOffsetFrameTranslationScalingStep,
        ThinPlateSplineMeshSubstitutionScalingStep,
        ThinPlateSplineWrapCylinderScalingStep,
        ModelMassToSubjectMassScalingStep,
        ManuallyScaleBodySegmentsScalingStep,
        RecalculateWrapCylinderRadiusFromStationScalingStep,
        RecalculateWrapCylinderXYZBodyRotationFromStationScalingStep
    >;

    // Returns a list of `ScalingStep` prototypes, so that downstream code is able to present
    // them as available options etc.
    const auto& getScalingStepPrototypes()
    {
        static const auto s_ScalingStepPrototypes = []<typename... TScalingStep>(Typelist<TScalingStep...>)
        {
            return std::to_array<std::unique_ptr<ScalingStep>>({
                std::make_unique<TScalingStep>()...
            });
        }(AllScalingStepTypes{});

        return s_ScalingStepPrototypes;
    }

    // Top-level document that describes a sequence of `ScalingStep`s that can be applied to
    // the source model in order to yield a scaled model.
    class ModelWarperV3Document final :
        public OpenSim::Component,
        public IVersionedComponentAccessor {
        OpenSim_DECLARE_CONCRETE_OBJECT(ModelWarperV3Document, OpenSim::Component)

        OpenSim_DECLARE_LIST_PROPERTY(scaling_parameter_overrides, ScalingParameterOverride, "A sequence of `ScalingParameterOverride`s that should be used in place of the default values used by the `ScalingStep`s.");
    public:
        ModelWarperV3Document()
        {
            constructProperty_scaling_parameter_overrides();
        }

        bool hasScalingSteps() const
        {
            if (getNumImmediateSubcomponents() == 0) {
                return false;
            }
            const auto lst = getComponentList<ScalingStep>();
            return lst.begin() != lst.end();
        }

        auto iterateScalingSteps() const
        {
            return getComponentList<ScalingStep>();
        }

        void addScalingStep(std::unique_ptr<ScalingStep> step)
        {
            addComponent(step.release());
            clearConnections();
            finalizeConnections(*this);
            finalizeFromProperties();
        }

        bool removeScalingStep(ScalingStep& step)
        {
            if (not step.hasOwner()) {
                return false;
            }
            if (&step.getOwner() != this) {
                return false;
            }

            auto& componentsProp = updProperty_components();
            if (int idx = componentsProp.findIndex(step); idx != -1) {
                componentsProp.removeValueAtIndex(idx);
            }

            clearConnections();
            finalizeConnections(*this);
            finalizeFromProperties();
            return true;
        }

        bool hasScalingParameters() const
        {
            if (not hasScalingSteps()) {
                return false;
            }
            for (const ScalingStep& step : iterateScalingSteps()) {
                bool called = false;
                step.forEachScalingParameterDeclaration([&called](const ScalingParameterDeclaration&) { called = true; });
                if (called) {
                    return true;
                }
            }
            return false;
        }

        ScalingParameters getEffectiveScalingParameters() const
        {
            ScalingParameters rv;

            // Get/merge values from the scaling steps
            for (const ScalingStep& step : iterateScalingSteps()) {
                step.forEachScalingParameterDeclaration([&step, &rv](const ScalingParameterDeclaration& decl)
                {
                    const auto [it, inserted] = rv.try_emplace(decl.name(), decl.default_value());
                    if (not inserted and it->second != decl.default_value()) {
                        std::stringstream msg;
                        msg << step.getAbsolutePath() << ": declares a scaling parameter (" << decl.name() << ") that has the same name as another scaling parameter, but they differ: the engine cannot figure out how to rectify this difference. The parameter should have a different name, or a disambiguating prefix added to it";
                        throw std::runtime_error{std::move(msg).str()};
                    }
                });
            }

            // Apply overrides to the effective scaling parameters
            {
                const OpenSim::Property<ScalingParameterOverride>& overrides = getProperty_scaling_parameter_overrides();
                for (int i = 0; i < overrides.size(); ++i) {
                    const ScalingParameterOverride& o = overrides.getValue(i);
                    rv.insert_or_assign(o.get_parameter_name(), o.get_parameter_value());
                }
            }

            return rv;
        }

        bool setScalingParameterOverride(const std::string& scalingParamName, ScalingParameterValue newValue)
        {
            mutateScalingParammeterOverridesWithNewOverride(scalingParamName, newValue);
            finalizeFromProperties();
            return true;
        }

        void saveTo(const std::filesystem::path& p) const
        {
            print(p.string());
        }

    private:
        void mutateScalingParammeterOverridesWithNewOverride(const std::string& scalingParamName, ScalingParameterValue newValue)
        {
            // First, try to find an existing override with the same name and overwrite it
            const OpenSim::Property<ScalingParameterOverride>& overrides = getProperty_scaling_parameter_overrides();
            for (int i = 0; i < overrides.size(); ++i) {
                const ScalingParameterOverride& o = overrides.getValue(i);
                if (o.get_parameter_name() == scalingParamName) {
                    updProperty_scaling_parameter_overrides().updValue(i).set_parameter_value(newValue);
                    return;  // found and overwritten
                }
            }

            // Otherwise, add a new override
            int idx = updProperty_scaling_parameter_overrides().appendValue(ScalingParameterOverride{scalingParamName, newValue});
            updProperty_scaling_parameter_overrides().updValue(idx).set_parameter_name(scalingParamName);
            updProperty_scaling_parameter_overrides().updValue(idx).set_parameter_value(newValue);
        }

        const OpenSim::Component& implGetComponent() const final
        {
            return *this;
        }

        bool implCanUpdComponent() const final
        {
            return true;
        }

        OpenSim::Component& implUpdComponent() final
        {
            throw std::runtime_error{ "component updating not implemented for this IComponentAccessor" };
        }
    };

    // A top-level message produced by validating an entire scaling document (not just `ScalingStep`s,
    // but any top-level validation concerns also). In MVC parlance, this is the M - so it shouldn't
    // directly use or refer to the UI.
    struct ScalingDocumentValidationMessage {
        OpenSim::ComponentPath sourceScalingStepAbsPath;
        ScalingStepValidationMessage payload;
    };

    // Top-level input state that's required to actually perform model scaling.
    class ScalingState final {  // NOLINT(cppcoreguidelines-special-member-functions,hicpp-special-member-functions)
    public:
        explicit ScalingState()
        {
            scalingDocument->finalizeConnections(*scalingDocument);
            scalingDocument->finalizeFromProperties();
        }

        ScalingState(const ScalingState& other) :
            sourceModel{std::make_shared<BasicModelStatePair>(*other.sourceModel)},
            scalingDocument{std::make_shared<ModelWarperV3Document>(*other.scalingDocument)}
        {
            // care: separate `ScalingState`s should act like separate instances with no
            //       reference sharing between them, but the shared pointers in the "main"
            //       `ScalingState` might already be divvied out to UI components, so we
            //       can't just switch the pointers around.
            scalingDocument->clearConnections();
            scalingDocument->finalizeConnections(*scalingDocument);
            scalingDocument->finalizeFromProperties();
        }

        ScalingState& operator=(const ScalingState& other)
        {
            // care: separate `ScalingState`s should act like separate instances with no
            //       reference sharing between them, but the shared pointers in the "main"
            //       `ScalingState` might already be divvied out to UI components, so we
            //       can't just switch the pointers around.
            if (&other == this) {
                return *this;
            }
            *sourceModel = *other.sourceModel;
            *scalingDocument = *other.scalingDocument;
            scalingDocument->clearConnections();
            scalingDocument->finalizeConnections(*scalingDocument);
            scalingDocument->finalizeFromProperties();
            return *this;
        }

        ~ScalingState() noexcept = default;

    // Source Model Methods

        const BasicModelStatePair& getSourceModel() const { return *sourceModel; }
        std::shared_ptr<BasicModelStatePair> getSourceModelPtr() { return sourceModel; }
        void loadSourceModelFromOsim(const std::filesystem::path& path)
        {
            App::singleton<RecentFiles>()->push_back(path);
            sourceModel = std::make_shared<BasicModelStatePair>(path);
        }
        void resetSourceModel()
        {
            sourceModel = std::make_shared<BasicModelStatePair>();
        }

    // Scaling Document Methods

        std::shared_ptr<const ModelWarperV3Document> getScalingDocumentPtr() const { return scalingDocument; }
        bool hasScalingSteps() const { return scalingDocument->hasScalingSteps(); }
        auto iterateScalingSteps() const { return scalingDocument->iterateScalingSteps(); }
        void addScalingStep(std::unique_ptr<ScalingStep> step)
        {
            scalingDocument->addScalingStep(std::move(step));
        }
        bool eraseScalingStep(ScalingStep& step)
        {
            return scalingDocument->removeScalingStep(step);
        }
        bool eraseScalingStep(const OpenSim::ComponentPath& path)
        {
            if (auto* scalingStep = findScalingComponentMut<ScalingStep>(path)) {
                return eraseScalingStep(*scalingStep);
            }
            else {
                return false;
            }
        }
        void applyScalingObjectPropertyEdit(ObjectPropertyEdit edit)
        {
            OpenSim::Component* component = findScalingComponentMut(edit.getComponentAbsPath());
            if (not component) {
                return;
            }
            OpenSim::AbstractProperty* property = FindPropertyMut(*component, edit.getPropertyName());
            if (not property) {
                return;
            }
            edit.apply(*property);
            scalingDocument->clearConnections();
            scalingDocument->finalizeConnections(*scalingDocument);
            scalingDocument->finalizeFromProperties();
        }
        bool disableScalingStep(const OpenSim::ComponentPath& path)
        {
            if (auto* scalingStep = findScalingComponentMut<ScalingStep>(path)) {
                scalingStep->set_enabled(false);
                scalingDocument->clearConnections();
                scalingDocument->finalizeConnections(*scalingDocument);
                scalingDocument->finalizeFromProperties();
                return true;
            }
            else {
                return false;
            }
        }
        std::vector<ScalingDocumentValidationMessage> getEnabledScalingStepValidationMessages(ScalingCache& scalingCache) const
        {
            std::vector<ScalingDocumentValidationMessage> rv;

            if (not hasScalingSteps()) {
                return rv;
            }

            const ScalingParameters scalingParameters = getEffectiveScalingParameters();

            for (const auto& scalingStep : scalingDocument->getComponentList<ScalingStep>()) {
                if (not scalingStep.get_enabled()) {
                    // Only aggregate validation errors from enabled `ScalingStep`s at the document-level.
                    continue;
                }
                auto stepMessages = scalingStep.validate(scalingCache, scalingParameters, *sourceModel);
                rv.reserve(rv.size() + stepMessages.size());
                for (auto& stepMessage : stepMessages) {
                    rv.push_back(ScalingDocumentValidationMessage{
                        .sourceScalingStepAbsPath = scalingStep.getAbsolutePath(),
                        .payload = std::move(stepMessage),
                    });
                }
            }
            return rv;
        }
        bool hasScalingStepValidationIssues(ScalingCache& scalingCache) const
        {
            return not getEnabledScalingStepValidationMessages(scalingCache).empty();
        }
        void resetScalingDocument()
        {
            scalingDocument = std::make_shared<ModelWarperV3Document>();
            scalingDocument->finalizeConnections(*scalingDocument);
            scalingDocument->finalizeFromProperties();
        }
        void loadScalingDocument(const std::filesystem::path& path)
        {
            const auto ptr = std::shared_ptr<OpenSim::Object>{OpenSim::Object::makeObjectFromFile(path.string())};
            if (auto downcasted = std::dynamic_pointer_cast<ModelWarperV3Document>(ptr)) {
                scalingDocument = std::move(downcasted);
                scalingDocument->finalizeConnections(*scalingDocument);
                scalingDocument->finalizeFromProperties();
            }
            else {
                std::stringstream ss;
                ss << path.string() << ": is a valid object file, but doesn't contain a ModelWarperV3Document";
                throw std::runtime_error{std::move(ss).str()};
            }
        }
        std::optional<std::filesystem::path> scalingDocumentFilesystemLocation() const
        {
            if (const auto filename = scalingDocument->getDocumentFileName(); not filename.empty()) {
                return std::filesystem::path{filename};
            }
            else {
                return std::nullopt;
            }
        }

        bool hasScalingParameterDeclarations() const { return scalingDocument->hasScalingParameters(); }
        ScalingParameters getEffectiveScalingParameters() const { return scalingDocument->getEffectiveScalingParameters(); }
        bool setScalingParameterOverride(const std::string& scalingParamName, ScalingParameterValue newValue)
        {
            return scalingDocument->setScalingParameterOverride(scalingParamName, newValue);
        }

    // Model Scaling

        // Tries to generate a scaled version of the source model using the current
        // scaling steps and scaling parameters.
        std::unique_ptr<BasicModelStatePair> tryGenerateScaledModel(ScalingCache& scalingCache) const
        {
            if (hasScalingStepValidationIssues(scalingCache)) {
                return nullptr;  // there are validation errors, so scaling isn't possible
            }

            // Create an independent copy of the source model, which will be scaled in-place.
            OpenSim::Model resultModel = sourceModel->getModel();
            resultModel.clearConnections();
            InitializeModel(resultModel);
            InitializeState(resultModel);

            if (not hasScalingSteps()) {
                // There are no scaling steps, so a copy of the source model is a scaled model (trivially).
                return std::make_unique<BasicModelStatePair>(std::move(resultModel));
            }

            // Calculate the effective scaling parameters (defaults + user-enacted overrides)
            const ScalingParameters scalingParams = getEffectiveScalingParameters();

            // Apply each scaling step to the scaled model
            for (auto& step : scalingDocument->updComponentList<ScalingStep>()) {
                step.applyScalingStep(scalingCache, scalingParams, *sourceModel, resultModel);
            }

            // set the current scaled model as the result model
            return std::make_unique<BasicModelStatePair>(std::move(resultModel));
        }

    private:
        template<typename T = OpenSim::Component>
        T* findScalingComponentMut(const OpenSim::ComponentPath& p) { return FindComponentMut<T>(*scalingDocument, p); }

        std::shared_ptr<BasicModelStatePair> sourceModel = std::make_shared<BasicModelStatePair>();
        std::shared_ptr<ModelWarperV3Document> scalingDocument = std::make_shared<ModelWarperV3Document>();
    };
}

// Controller datastructures (i.e. middleware between the UI and the underlying model).
namespace
{
    // Top-level UI state that's shared between panels/widget that the UI manipulates.
    class ModelWarperV3UIState final : public std::enable_shared_from_this<ModelWarperV3UIState> {
    public:

        explicit ModelWarperV3UIState(Widget* parent) :
            parent_{parent}
        {}

        // Should be regularly called by the UI once per frame.
        void on_tick()
        {
            try {
                if (not m_DeferredActions.empty()) {
                    for (auto& deferredAction : m_DeferredActions) {
                        deferredAction(*this);
                    }
                    m_DeferredActions.clear();

                    updateScaledModel();
                }
            }
            catch (const std::exception& ex) {
                log_error("error processing deferred actions: %s", ex.what());
            }
        }

        // Returns a shared readonly pointer to the top-level model warping document.
        std::shared_ptr<const ModelWarperV3Document> getDocumentPtr() const
        {
            return m_ScalingState->scratch().getScalingDocumentPtr();
        }


        bool hasScalingSteps() const
        {
            return m_ScalingState->scratch().hasScalingSteps();
        }
        auto iterateScalingSteps() const
        {
            return m_ScalingState->scratch().iterateScalingSteps();
        }
        void addScalingStepDeferred(std::unique_ptr<ScalingStep> step)
        {
            m_DeferredActions.emplace_back([s = std::shared_ptr<ScalingStep>{std::move(step)}](ModelWarperV3UIState& state) mutable
            {
                state.m_ScalingState->upd_scratch().addScalingStep(std::unique_ptr<ScalingStep>{s->clone()});
                state.m_ScalingState->commit_scratch("Add scaling step");
            });
        }
        void eraseScalingStepDeferred(const ScalingStep& step)
        {
            m_DeferredActions.emplace_back([path = step.getAbsolutePath()](ModelWarperV3UIState& state)
            {
                if (state.m_ScalingState->upd_scratch().eraseScalingStep(path)) {
                    state.m_ScalingState->commit_scratch("Erase scaling step");
                }
            });
        }
        std::vector<ScalingStepValidationMessage> validateStep(const ScalingStep& step)
        {
            return step.validate(
                m_ScalingCache,
                m_ScalingState->scratch().getEffectiveScalingParameters(),
                m_ScalingState->scratch().getSourceModel()
            );
        }


        bool hasScalingParameters() const
        {
            return m_ScalingState->scratch().hasScalingParameterDeclarations();
        }
        ScalingParameters getEffectiveScalingParameters() const
        {
            return m_ScalingState->scratch().getEffectiveScalingParameters();
        }
        void setScalingParameterValueDeferred(const std::string& scalingParamName, ScalingParameterValue newValue)
        {
            m_DeferredActions.emplace_back([scalingParamName, newValue](ModelWarperV3UIState& state)
            {
                if (state.m_ScalingState->upd_scratch().setScalingParameterOverride(scalingParamName, newValue)) {
                    state.m_ScalingState->commit_scratch("Set scaling parameter");
                }
            });
        }


        std::shared_ptr<IModelStatePair> sourceModel()
        {
            return m_ScalingState->upd_scratch().getSourceModelPtr();
        }


        // result model stuff (note: might not be available if there's validation issues)
        using ScaledModelOrValidationErrorsOrScalingErrors = std::variant<
            std::shared_ptr<IModelStatePair>,               // successfully scaled model
            std::vector<ScalingDocumentValidationMessage>,  // vaidation messages
            std::string                                     // runtime scaling error message (i.e. after validation passed but during sclaing)
        >;
        ScaledModelOrValidationErrorsOrScalingErrors scaledModelOrDocumentValidationMessages()
        {
            if (m_ScalingErrorMessage) {
                return m_ScalingErrorMessage.value();
            }
            else if (auto validationMessages = m_ScalingState->scratch().getEnabledScalingStepValidationMessages(m_ScalingCache); not validationMessages.empty()) {
                return validationMessages;
            }
            else {
                return m_ScaledModel;
            }
        }

        bool canExportWarpedModel() const
        {
            return not m_ScalingErrorMessage.has_value();
        }

        void exportWarpedModelToModelEditor()
        {
            if (not canExportWarpedModel()) {
                return;  // can't warp
            }

            auto scalingResult = scaledModelOrDocumentValidationMessages();
            if (not std::holds_alternative<std::shared_ptr<IModelStatePair>>(scalingResult)) {
                log_error("cannot export scaled model: could not create a warped model");
                return;
            }

            const OpenSim::Model& sourceModel = m_ScalingState->scratch().getSourceModel();
            const auto modelFilesystemLocation = TryFindInputFile(sourceModel);
            if (not modelFilesystemLocation) {
                log_error("cannot export scaled model: can't figure out where the source model is on-disk");
                return;
            }

            std::shared_ptr<IModelStatePair> scaled = std::get<std::shared_ptr<IModelStatePair>>(scalingResult);

            std::unique_ptr<OpenSim::Model> copy = std::make_unique<OpenSim::Model>(scaled->getModel());
            InitializeModel(*copy);
            InitializeState(*copy);
            {
                // TODO/FIXME/HACK: this code was thrown together to solve an immediate problem
                // of being able to export warped models, but it isn't very clean or robust (#1003).
                auto inMemoryMeshes = scaled->getModel().getComponentList<InMemoryMesh>();
                if (inMemoryMeshes.begin() != inMemoryMeshes.end()) {
                    const auto warpedGeometryDir = tryGetWarpedGeometryDirectory();
                    if (not warpedGeometryDir) {
                        log_error("cannot export scaled model: can't figure out where to save the warped meshes");
                        return;
                    }

                    // If the warped geometry directory doesn't exist yet, create it.
                    if (not std::filesystem::exists(*warpedGeometryDir)) {
                        std::filesystem::create_directories(*warpedGeometryDir);
                    }

                    // Export `InMemoryMesh`es to disk (#1003)
                    for (const InMemoryMesh& mesh : inMemoryMeshes) {
                        // Figure out output file name.
                        const auto& inputMesh = sourceModel.getComponent<OpenSim::Mesh>(mesh.getAbsolutePath());
                        const auto warpedMeshAbsPath = std::filesystem::weakly_canonical(*warpedGeometryDir / std::filesystem::path{inputMesh.get_mesh_file()}.filename().replace_extension(".obj"));

                        // Write warped mesh data to disk in an OBJ format.
                        {
                            std::ofstream objStream{warpedMeshAbsPath, std::ios::trunc};
                            objStream.exceptions(std::ios::badbit | std::ios::failbit);
                            OBJ::write(objStream, mesh.getOscMesh(), OBJMetadata{"osc-model-warper"});
                        }

                        // Overwrite the `InMemoryMesh` in `copy`
                        auto& copyMesh = copy->updComponent<InMemoryMesh>(mesh.getAbsolutePath());
                        auto newMesh = std::make_unique<OpenSim::Mesh>();
                        newMesh->set_mesh_file(warpedMeshAbsPath.string());
                        OverwriteGeometry(*copy, copyMesh, std::move(newMesh));
                    }
                }
            }

            // Make the model undo-able
            auto undoableModel = std::make_unique<UndoableModelStatePair>(std::move(copy));

            // If the user requests it, bake the SDFs after finishing everything else
            if (getBakeSDFs()) {
                ActionBakeStationDefinedFrames(*undoableModel);
            }

            auto editor = std::make_unique<ModelEditorTab>(parent_, std::move(undoableModel));
            App::post_event<OpenTabEvent>(*parent_, std::move(editor));
        }

        // camera stuff
        bool isCameraLinked() const { return m_LinkCameras; }
        void setCameraLinked(bool v) { m_LinkCameras = v; }
        bool isOnlyCameraRotationLinked() const { return m_OnlyLinkRotation; }
        void setOnlyCameraRotationLinked(bool v) { m_OnlyLinkRotation = v; }
        const PolarPerspectiveCamera& getLinkedCamera() const { return m_LinkedCamera; }
        void setLinkedCamera(const PolarPerspectiveCamera& camera) { m_LinkedCamera = camera; }


        // undo/redo stuff
        std::shared_ptr<UndoRedoBase> getUndoRedoPtr() { return m_ScalingState; }


        // warped geometry dir stuff
        std::optional<std::filesystem::path> tryGetWarpedGeometryDirectory() const
        {
            if (m_MaybeCustomWarpedGeometryDirectory) {
                return m_MaybeCustomWarpedGeometryDirectory;  // top-prio is user-enacted choice (#1046).
            }

            const OpenSim::Model& sourceModel = m_ScalingState->scratch().getSourceModel();
            const auto modelFilesystemLocation = TryFindInputFile(sourceModel);
            if (modelFilesystemLocation) {
                return modelFilesystemLocation->parent_path() / "WarpedGeometry";
            }

            return std::nullopt;  // can't figure out where to save it :(
        }

        // SDF baking stuff
        bool getBakeSDFs() const { return m_BakeStationDefinedFrames; }
        void setBakeSDFs(bool v) { m_BakeStationDefinedFrames = v; }

        // actions
        void actionCreateNewSourceModel()
        {
            m_ScalingState->upd_scratch().resetSourceModel();
            updateScaledModel();
            m_ScalingState->commit_scratch("Create new source model");
        }

        void actionOpenOsimOrPromptUser(std::optional<std::filesystem::path> path)
        {
            if (path) {
                actionOpenOsim(*path);
            }
            else {
                if (not shared_from_this()) {
                    log_critical("can't open osim file selection dialog because the UI state isn't shared");
                    return;
                }

                App::upd().prompt_user_to_select_file_async(
                    [state = shared_from_this()](const FileDialogResponse& response)
                    {
                        if (not state) {
                            return;  // Something bad has happened.
                        }
                        if (response.size() != 1) {
                            return;  // Error, cancellation, or the user somehow selected >1 file.
                        }
                        state->actionOpenOsim(response.front());
                    },
                    GetModelFileFilters()
                );
            }
        }

        void actionPromptUserToSelectWarpedGeometryDirectory()
        {
            App::upd().prompt_user_to_select_directory_async(
                [state = shared_from_this()](const FileDialogResponse& response)
                {
                    if (not state) {
                        return;  // Can't continue.
                    }
                    if (response.size() != 1) {
                        return;  // Error, cancellation, or the user somehow selected >1 file.
                    }

                    state->m_MaybeCustomWarpedGeometryDirectory = response.front();
                }
            );
        }

        void actionOpenOsim(const std::filesystem::path& path)
        {
            App::singleton<RecentFiles>()->push_back(path);
            m_ScalingState->upd_scratch().loadSourceModelFromOsim(path);
            updateScaledModel();
            m_ScalingState->commit_scratch("Loaded osim file");
        }

        void actionCreateNewScalingDocument()
        {
            m_ScalingState->upd_scratch().resetScalingDocument();
            updateScaledModel();
            m_ScalingState->commit_scratch("Create new scaling document");
        }

        void actionOpenScalingDocument()
        {
            if (not shared_from_this()) {
                log_critical("cannot open a file selection dialog because the UI state isn't reference-counted");
                return;
            }

            App::upd().prompt_user_to_select_file_async(
                [state = shared_from_this()](const FileDialogResponse& response)
                {
                    if (not state) {
                        return;  // Can't continue.
                    }
                    if (response.size() != 1) {
                        return;  // Error, cancellation, or the user somehow selected >1 file.
                    }

                    state->m_ScalingState->upd_scratch().loadScalingDocument(response.front());
                    state->updateScaledModel();
                    state->m_ScalingState->commit_scratch("Loaded scaling document");
                },
                GetOpenSimXMLFileFilters()
            );
        }

        void actionSaveScalingDocument()
        {
            if (const auto existingPath = m_ScalingState->scratch().scalingDocumentFilesystemLocation()) {
                m_ScalingState->upd_scratch().getScalingDocumentPtr()->saveTo(*existingPath);
                return;  // Document saved to existing filesystem location.
            }

            // Else: prompt the user to save it
            App::upd().prompt_user_to_save_file_with_extension_async([doc = m_ScalingState->upd_scratch().getScalingDocumentPtr()](std::optional<std::filesystem::path> p)
            {
                if (not p) {
                    return;  // user cancelled out of the prompt
                }
                doc->saveTo(*p);
            }, "xml");
        }

        void actionApplyObjectEditToScalingDocument(const ObjectPropertyEdit& edit)
        {
            m_ScalingState->upd_scratch().applyScalingObjectPropertyEdit(edit);
            updateScaledModel();
            m_ScalingState->commit_scratch("change scaling property");
        }

        void actionDisableScalingStep(const OpenSim::ComponentPath& path)
        {
            m_ScalingState->upd_scratch().disableScalingStep(path);
            updateScaledModel();
            m_ScalingState->commit_scratch("disable scaling step");
        }

        void actionRollback()
        {
            m_ScalingState->rollback();
        }

        void actionRetryScalingDeferred()
        {
            m_DeferredActions.emplace_back([](ModelWarperV3UIState& state) mutable
            {
                state.updateScaledModel();
            });
        }

        bool canUndo() const
        {
            return m_ScalingState->can_undo();
        }
        void actionUndo()
        {
            m_ScalingState->undo();
        }
    private:
        void updateScaledModel()
        {
            try {
                auto scaledModel = m_ScalingState->scratch().tryGenerateScaledModel(m_ScalingCache);
                if (not scaledModel) {
                    return;
                }

                m_ScaledModel = std::move(scaledModel);
                m_ScalingErrorMessage.reset();
            }
            catch (const std::exception& ex) {
                m_ScalingErrorMessage = ex.what();
            }
        }

        Widget* parent_ = nullptr;

        std::shared_ptr<UndoRedo<ScalingState>> m_ScalingState = std::make_shared<UndoRedo<ScalingState>>();

        ScalingCache m_ScalingCache;
        std::shared_ptr<BasicModelStatePair> m_ScaledModel = std::make_shared<BasicModelStatePair>();
        std::optional<std::string> m_ScalingErrorMessage;

        std::vector<std::function<void(ModelWarperV3UIState&)>> m_DeferredActions;
        bool m_LinkCameras = true;
        bool m_OnlyLinkRotation = false;
        PolarPerspectiveCamera m_LinkedCamera;

        std::optional<std::filesystem::path> m_MaybeCustomWarpedGeometryDirectory;
        bool m_BakeStationDefinedFrames = false;
    };
}

namespace
{
    Color ui_color(const ScalingStepValidationMessage& message)
    {
        switch (message.getState()) {
        case ScalingStepValidationState::Warning: return Color::orange();
        case ScalingStepValidationState::Error:   return Color::muted_red();
        default:                                  return Color::muted_red();
        }
    }

    // source model 3D viewer
    class ModelWarperV3SourceModelViewerPanel final : public ModelViewerPanel {
    public:
        explicit ModelWarperV3SourceModelViewerPanel(
            Widget* parent,
            std::string_view label,
            std::shared_ptr<ModelWarperV3UIState> state) :

            ModelViewerPanel{parent, label, ModelViewerPanelParameters{state->sourceModel()}, ModelViewerPanelFlag::NoHittest},
            m_State{std::move(state)}
        {}

    private:
        void impl_draw_content() final
        {
            if (m_State->isCameraLinked()) {
                if (m_State->isOnlyCameraRotationLinked()) {
                    auto camera = getCamera();
                    camera.phi = m_State->getLinkedCamera().phi;
                    camera.theta = m_State->getLinkedCamera().theta;
                    setCamera(camera);
                }
                else {
                    setCamera(m_State->getLinkedCamera());
                }
            }

            setModelState(m_State->sourceModel());
            ModelViewerPanel::impl_draw_content();

            // draw may have updated the camera, so flash is back
            if (m_State->isCameraLinked()) {
                if (m_State->isOnlyCameraRotationLinked()) {
                    auto camera = m_State->getLinkedCamera();
                    camera.phi = getCamera().phi;
                    camera.theta = getCamera().theta;
                    m_State->setLinkedCamera(camera);
                }
                else {
                    m_State->setLinkedCamera(getCamera());
                }
            }
        }

        std::shared_ptr<ModelWarperV3UIState> m_State;
    };

    // result model 3D viewer
    class ModelWarperV3ResultModelViewerPanel final : public ModelViewerPanel {
    public:
        ModelWarperV3ResultModelViewerPanel(
            Widget* parent,
            std::string_view label,
            std::shared_ptr<ModelWarperV3UIState> state) :

            ModelViewerPanel{parent, label, ModelViewerPanelParameters{state->sourceModel()}, ModelViewerPanelFlag::NoHittest},
            m_State{std::move(state)}
        {}

    private:
        void impl_draw_content() final
        {
            std::visit(Overload{
                [this](const std::shared_ptr<IModelStatePair>& scaledModel) { draw_scaled_model_visualization(scaledModel); },
                [this](const std::vector<ScalingDocumentValidationMessage>& messages) { draw_validation_error_message(messages); },
                [this](const std::string& scalingErrorMessage) { draw_scaling_error_message(scalingErrorMessage); },
            }, m_State->scaledModelOrDocumentValidationMessages());
        }

        void draw_scaled_model_visualization(const std::shared_ptr<IModelStatePair>& scaledModel)
        {
            // handle camera linking
            if (m_State->isCameraLinked()) {
                if (m_State->isOnlyCameraRotationLinked()) {
                    auto camera = getCamera();
                    camera.phi = m_State->getLinkedCamera().phi;
                    camera.theta = m_State->getLinkedCamera().theta;
                    setCamera(camera);
                }
                else {
                    setCamera(m_State->getLinkedCamera());
                }
            }

            setModelState(scaledModel);
            ModelViewerPanel::impl_draw_content();

            // draw may have updated the camera, so flash is back
            if (m_State->isCameraLinked()) {
                if (m_State->isOnlyCameraRotationLinked()) {
                    auto camera = m_State->getLinkedCamera();
                    camera.phi = getCamera().phi;
                    camera.theta = getCamera().theta;
                    m_State->setLinkedCamera(camera);
                }
                else {
                    m_State->setLinkedCamera(getCamera());
                }
            }
        }

        void draw_validation_error_message(std::span<const ScalingDocumentValidationMessage> messages)
        {
            const float contentHeight = static_cast<float>(messages.size() + 2) * ui::get_text_line_height_in_current_panel();
            const float regionHeight = ui::get_content_region_available().y;
            const float top = 0.5f * (regionHeight - contentHeight);

            ui::set_cursor_panel_position({0.0f, top});

            // header line
            {
                std::stringstream ss;
                ss << "Cannot show model: " << messages.size() << " validation error" << (messages.size() > 1 ? "s" : "") << " detected:";
                ui::draw_text_centered(std::move(ss).str());
            }

            // error line(s)
            int id = 0;
            for (const auto& message : messages) {
                ui::push_id(id++);

                ui::push_style_color(ui::ColorVar::Text, ui_color(message.payload));
                std::stringstream ss;
                ss << message.sourceScalingStepAbsPath.getComponentName() << ": " << message.payload.getMessage();
                ui::draw_text_bullet_pointed(std::move(ss).str());
                ui::pop_style_color();

                ui::same_line();
                if (ui::draw_small_button("Disable Scaling Step")) {
                    m_State->actionDisableScalingStep(message.sourceScalingStepAbsPath);
                }

                ui::pop_id();
            }
        }

        void draw_scaling_error_message(CStringView message)
        {
            const float h = ui::get_content_region_available().y;
            const float lineHeight = ui::get_text_line_height_in_current_panel();
            constexpr float numLines = 3.0f;
            const float top = 0.5f * (h - numLines*lineHeight);

            ui::set_cursor_panel_position({0.0f, top});
            ui::draw_text_centered("An error occured while trying to scale the model:");
            ui::draw_text_centered(message);
            if (ui::draw_button_centered(OSC_ICON_RECYCLE " Retry Scaling")) {
                m_State->actionRetryScalingDeferred();
            }
        }

        std::shared_ptr<ModelWarperV3UIState> m_State;
    };

    class WarpedModelExporterPopup final : public Popup {
    public:
        explicit WarpedModelExporterPopup(Widget* parent) :
            Popup{parent, "hello, popup world!"}
        {}

    private:
        void impl_draw_content() final
        {
            ui::draw_text("hello, popup content!");
        }
    };

    // main toolbar
    class ModelWarperV3Toolbar final : public Widget {
    public:
        explicit ModelWarperV3Toolbar(
            Widget* parent,
            std::string_view label,
            std::shared_ptr<ModelWarperV3UIState> state) :
            Widget{parent},
            m_State{std::move(state)}
        {
            set_name(label);
        }
    private:
        void impl_on_draw() final
        {
            if (BeginToolbar(name())) {
                draw_content();
            }
            ui::end_panel();
        }

        void draw_content()
        {
            int id = 0;

            ui::push_id(id++);
            ui::draw_vertical_separator();
            ui::same_line();
            ui::draw_text("Source Model: ");
            ui::same_line();
            if (ui::draw_button(OSC_ICON_FILE)) {
                m_State->actionCreateNewSourceModel();
            }
            ui::same_line();
            DrawOpenModelButtonWithRecentFilesDropdown([this](auto maybeSelection)
            {
                m_State->actionOpenOsimOrPromptUser(std::move(maybeSelection));
            });
            ui::same_line();
            ui::draw_vertical_separator();
            ui::pop_id();


            ui::push_id(id++);
            ui::same_line();
            ui::draw_text("Scaling Document: ");
            ui::same_line();
            if (ui::draw_button(OSC_ICON_FILE)) {
                m_State->actionCreateNewScalingDocument();
            }
            ui::same_line();
            if (ui::draw_button(OSC_ICON_FOLDER_OPEN)) {
                m_State->actionOpenScalingDocument();
            }
            ui::same_line();
            if (ui::draw_button(OSC_ICON_SAVE)) {
                m_State->actionSaveScalingDocument();
            }
            ui::same_line();
            ui::draw_vertical_separator();
            ui::pop_id();


            ui::push_id(id++);
            ui::same_line();
            m_UndoButton.on_draw();
            ui::pop_id();
            ui::push_id(id++);
            ui::same_line();
            m_RedoButton.on_draw();
            ui::same_line();
            ui::draw_vertical_separator();
            ui::pop_id();

            // Draw green "Warp Model" button
            {
                ui::push_id(id++);
                ui::same_line();
                bool disabled = false;
                if (not m_State->canExportWarpedModel()) {
                    ui::begin_disabled();
                    disabled = true;
                }

                ui::push_style_color(ui::ColorVar::Button, Color::dark_green());
                if (ui::draw_button(OSC_ICON_PLAY " Export Warped Model")) {
                    m_State->exportWarpedModelToModelEditor();
                }
                ui::add_screenshot_annotation_to_last_drawn_item("Export Warped Model Button");
                ui::pop_style_color();
                ui::same_line(0.0f, 1.0f);
                if (ui::draw_button(OSC_ICON_COG)) {
                    ui::open_popup("##WarpedModelExportModifiers");
                }


                if (ui::begin_popup("##WarpedModelExportModifiers")) {

                    // UI for warped geometry directory toggle
                    const auto geomDir = m_State->tryGetWarpedGeometryDirectory();
                    if (geomDir) {
                        ui::draw_text("Warped Geometry Directory: %s", geomDir->string().c_str());
                    }
                    else {
                        ui::draw_text("Warped Geometry Directory: UNKNOWN");
                    }
                    ui::same_line();
                    if (ui::draw_small_button("change")) {
                        m_State->actionPromptUserToSelectWarpedGeometryDirectory();
                    }


                    // UI for baking `StationDefinedFrame`s during warping
                    {
                        bool bakeSDFs = m_State->getBakeSDFs();
                        if (ui::draw_checkbox("Bake StationDefinedFrames", &bakeSDFs)) {
                            m_State->setBakeSDFs(bakeSDFs);
                        }
                    }

                    ui::end_popup();
                }
                if (disabled) {
                    ui::end_disabled();
                }
                ui::same_line();
                ui::draw_vertical_separator();
                ui::pop_id();
            }

            ui::push_id(id++);
            ui::same_line();
            if (bool v = m_State->isCameraLinked(); ui::draw_checkbox("link cameras", &v)) {
                m_State->setCameraLinked(v);
            }

            ui::same_line();
            if (bool v = m_State->isOnlyCameraRotationLinked(); ui::draw_checkbox("only link rotation", &v)) {
                m_State->setOnlyCameraRotationLinked(v);
            }
            ui::pop_id();
        }

        std::shared_ptr<ModelWarperV3UIState> m_State;
        UndoButton m_UndoButton{this, m_State->getUndoRedoPtr(), OSC_ICON_UNDO};
        RedoButton m_RedoButton{this, m_State->getUndoRedoPtr(), OSC_ICON_REDO};
    };

    // control panel (design, set parameters, etc.)
    class ModelWarperV3ControlPanel final : public Panel {
    public:
        explicit ModelWarperV3ControlPanel(
            Widget* parent,
            std::string_view panelName,
            std::shared_ptr<ModelWarperV3UIState> state) :

            Panel{parent, panelName},
            m_State{std::move(state)}
        {}

    private:
        void impl_draw_content() final
        {
            draw_scaling_parameters();
            ui::draw_vertical_spacer(0.75f);
            draw_scaling_steps();
        }

        void draw_scaling_parameters()
        {
            ui::draw_text_centered("Scaling Parameters");
            ui::draw_separator();
            ui::draw_vertical_spacer(0.5f);
            if (m_State->hasScalingParameters()) {
                if (ui::begin_table("##ScalingParameters", 2)) {
                    ui::table_setup_column("Name");
                    ui::table_setup_column("Value");
                    ui::table_headers_row();

                    int id = 0;
                    for (const auto& [name, value] : m_State->getEffectiveScalingParameters()) {
                        ui::push_id(id++);
                        ui::table_next_row();
                        ui::table_set_column_index(0);
                        ui::draw_text(name);
                        ui::table_set_column_index(1);
                        auto valueCopy{value};
                        if (ui::draw_double_input("##valueeditor", &valueCopy, 0.0, 0.0, "%.6f", ui::TextInputFlag::EnterReturnsTrue)) {
                            m_State->setScalingParameterValueDeferred(name, valueCopy);
                        }
                        ui::pop_id();
                    }
                    ui::end_table();
                }
            }
            else {
                ui::draw_text_disabled_and_centered("No Scaling Parameters.");
                ui::draw_text_disabled_and_centered("(scaling parameters are normally implicitly added by scaling steps)");
            }
        }

        void draw_scaling_steps()
        {
            ui::draw_text_centered("Scaling Steps");
            ui::draw_separator();
            ui::draw_vertical_spacer(0.5f);

            if (m_State->hasScalingSteps()) {
                size_t i = 0;
                for (const ScalingStep& step : m_State->iterateScalingSteps()) {
                    ui::push_id(step.getAbsolutePathString());
                    draw_scaling_step(i, step);
                    ui::pop_id();
                    ++i;
                }
            }
            else {
                ui::draw_text_disabled_and_centered("No scaling steps.");
                ui::draw_text_disabled_and_centered("(the model will be left unmodified)");
            }

            ui::draw_vertical_spacer(0.25f);
            draw_add_scaling_step_context_button();
        }

        void draw_scaling_step(size_t stepIndex, const ScalingStep& step)
        {
            // draw collapsing header, don't render content if it's collapsed
            {
                std::stringstream header;
                header << '#' << stepIndex + 1 << ": " << step.label();
                if (not ui::draw_collapsing_header(std::move(header).str(), ui::TreeNodeFlag::DefaultOpen)) {
                    return;  // header is collapsed
                }
            }
            // else: header isn't collapsed

            ui::draw_help_marker(step.getDescription());

            // draw deletion button
            {
                constexpr auto deletionButtonIcon = OSC_ICON_TRASH;

                ui::same_line();

                const Vector2 oldCursorPos = ui::get_cursor_panel_position();
                const float endX = oldCursorPos.x + ui::get_content_region_available().x;

                const Vector2 newCursorPos = {endX - ui::calc_button_size(deletionButtonIcon).x, oldCursorPos.y};
                ui::set_cursor_panel_position(newCursorPos);
                if (ui::draw_small_button(deletionButtonIcon)) {
                    m_State->eraseScalingStepDeferred(step);
                }
            }

            // draw validation messages
            {
                const auto messages = m_State->validateStep(step);
                if (not messages.empty()) {
                    ui::draw_vertical_spacer(0.2f);
                    ui::indent();
                    for (const ScalingStepValidationMessage& message : messages) {
                        ui::push_style_color(ui::ColorVar::Text, ui_color(message));
                        ui::draw_bullet_point();
                        if (const auto propName = message.tryGetPropertyName()) {
                            ui::draw_text("%s: %s", propName->c_str(), message.getMessage());
                        }
                        else {
                            ui::draw_text(message.getMessage());
                        }
                        ui::pop_style_color();
                    }
                    ui::unindent();
                    ui::draw_vertical_spacer(0.2f);
                }
            }

            // draw property editors
            ui::indent(1.0f*ui::get_text_line_height_in_current_panel());
            {
                const auto path = step.getAbsolutePathString();
                const auto docPtr = m_State->getDocumentPtr();
                const auto [it, inserted] = m_StepPropertyEditors.try_emplace(
                    path,
                    this,
                    docPtr,
                    [docPtr, path] { return FindComponent(*docPtr, path); }
                );
                if (inserted) {
                    it->second.insertInBlacklist("components");
                }
                if (auto objectEdit = it->second.onDraw()) {
                    m_State->actionApplyObjectEditToScalingDocument(std::move(objectEdit).value());
                }
            }
            ui::unindent(1.0f*ui::get_text_line_height_in_current_panel());
            ui::draw_vertical_spacer(0.5f);
        }

        void draw_add_scaling_step_context_button()
        {
            ui::draw_button(OSC_ICON_PLUS "Add Scaling Step", {ui::get_content_region_available().x, ui::calc_button_size("").y});
            if (ui::begin_popup_context_menu("##AddScalingStepPopupMenu", ui::PopupFlag::MouseButtonLeft)) {
                for (const auto& ptr : getScalingStepPrototypes()) {
                    ui::push_id(ptr.get());
                    if (ui::draw_selectable(ptr->label())) {
                        m_State->addScalingStepDeferred(std::unique_ptr<ScalingStep>{ptr->clone()});
                    }
                    ui::draw_tooltip_if_item_hovered(ptr->label(), ptr->getDescription(), ui::HoveredFlag::DelayNormal);
                    ui::pop_id();
                }
                ui::end_popup();
            }
        }

        std::shared_ptr<ModelWarperV3UIState> m_State;
        std::unordered_map<std::string, ObjectPropertiesEditor> m_StepPropertyEditors;
    };
}

class osc::ModelWarperTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "OpenSim/ModelWarperV3"; }

    explicit Impl(Tab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {
        // Ensure `ModelWarperV3Document` can be loaded from the filesystem via OpenSim.
        [[maybe_unused]] static const bool s_TypesRegistered = []<typename... TScalingStep>(Typelist<TScalingStep...>)
        {
            OpenSim::Object::registerType(ScalingParameterOverride{});
            (OpenSim::Object::registerType(TScalingStep{}), ...);
            OpenSim::Object::registerType(ModelWarperV3Document{});
            return true;
        }(AllScalingStepTypes{});

        m_PanelManager->register_toggleable_panel("Control Panel", [state = m_State](Widget* parent, std::string_view panelName)
        {
            return std::make_shared<ModelWarperV3ControlPanel>(parent, panelName, state);
        });
        m_PanelManager->register_toggleable_panel("Source Model", [state = m_State](Widget* parent, std::string_view panelName)
        {
            return std::make_shared<ModelWarperV3SourceModelViewerPanel>(parent, panelName, state);
        });
        m_PanelManager->register_toggleable_panel("Result Model", [state = m_State](Widget* parent, std::string_view panelName)
        {
            return std::make_shared<ModelWarperV3ResultModelViewerPanel>(parent, panelName, state);
        });
        m_PanelManager->register_toggleable_panel("Log", [](Widget* parent, std::string_view panelName)
        {
            return std::make_shared<LogViewerPanel>(parent, panelName);
        });
        m_PanelManager->register_toggleable_panel("Performance", [](Widget* parent, std::string_view panelName)
        {
            return std::make_shared<PerfPanel>(parent, panelName);
        });
    }

    void on_mount()
    {
        m_PanelManager->on_mount();
    }

    void on_unmount()
    {
        m_PanelManager->on_unmount();
    }

    void on_tick()
    {
        m_State->on_tick();
        m_PanelManager->on_tick();
    }

    void on_draw_main_menu()
    {
        m_WindowMenu.on_draw();
        m_AboutTab.onDraw();
    }

    void on_draw()
    {
        try {
            ui::enable_dockspace_over_main_window();
            m_PanelManager->on_draw();
            m_Toolbar.on_draw();
            m_ExceptionThrownLastFrame = false;
        } catch (const std::exception& ex) {
            log_error("an exception was thrown, probably due to an error in the document: %s", ex.what());

            // The exception might've left the UI context in an invalid state, so reset it.
            log_error("resetting the UI");
            App::notify<ResetUIContextEvent>(*parent());

            if (std::exchange(m_ExceptionThrownLastFrame, true)) {
                // An exception was also thrown last frame - try to undo the state to try
                // and fix the problem for the user.
                if (m_State->canUndo()) {
                    log_error("attempting to fix the problem by undo-ing the document");
                    m_State->actionUndo();
                }
                else {
                    // Undoing isn't possible, so we must defer this error to a higher level
                    // of the system.
                    log_critical("the document cannot be undone, elevating the exception");
                    throw;
                }
            }
            else {
                // No exception thrown last frame, it's likely that the exception is due to
                // an edit to the scratch space last frame, so try to softly roll the model
                // back.
                log_error("rolling back the document");
                m_State->actionRollback();
            }
        }
    }

private:
    std::shared_ptr<ModelWarperV3UIState> m_State = std::make_shared<ModelWarperV3UIState>(&this->owner());

    std::shared_ptr<PanelManager> m_PanelManager = std::make_shared<PanelManager>(&owner());
    WindowMenu m_WindowMenu{&owner(), m_PanelManager};
    MainMenuAboutTab m_AboutTab;
    ModelWarperV3Toolbar m_Toolbar{&this->owner(), "##ModelWarperV3Toolbar", m_State};

    bool m_ExceptionThrownLastFrame = false;
};

CStringView osc::ModelWarperTab::id() { return Impl::static_label(); }

osc::ModelWarperTab::ModelWarperTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::ModelWarperTab::impl_on_mount() { private_data().on_mount(); }
void osc::ModelWarperTab::impl_on_unmount() { private_data().on_unmount(); }
void osc::ModelWarperTab::impl_on_tick() { private_data().on_tick(); }
void osc::ModelWarperTab::impl_on_draw_main_menu() { private_data().on_draw_main_menu(); }
void osc::ModelWarperTab::impl_on_draw() { private_data().on_draw(); }

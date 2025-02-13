#include "ModelWarperV3Tab.h"

#include <libopensimcreator/Documents/CustomComponents/InMemoryMesh.h>
#include <libopensimcreator/Documents/Landmarks/LandmarkHelpers.h>
#include <libopensimcreator/Documents/Landmarks/MaybeNamedLandmarkPair.h>
#include <libopensimcreator/Documents/Model/BasicModelStatePair.h>
#include <libopensimcreator/Documents/Model/IModelStatePair.h>
#include <libopensimcreator/Graphics/OpenSimDecorationGenerator.h>
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
#include <OpenSim/OpenSim.h>

#include <algorithm>
#include <filesystem>
#include <memory>
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
    // Tries to delete an item from an `OpenSim::Set`.
    //
    // Returns `true` if the item was found and deleted; otherwise, returns `false`.
    template<typename T, typename TSetBase = OpenSim::Object>
    bool TryDeleteItemFromSet(OpenSim::Set<T, TSetBase>& set, const T* item)
    {
        for (size_t i = 0; i < size(set); ++i) {
            if (&At(set, i) == item) {
                return EraseAt(set, i);
            }
        }
        return false;
    }

    // Tries to overwride `oldGeometry` in the given `model` with `newGeometry`.
    //
    // This is useful when transforming geometry (e.g. TPS warping) and overwriting it
    // in a model.
    void OverwriteGeometry(
        OpenSim::Model& model,
        OpenSim::Geometry& oldGeometry,
        std::unique_ptr<OpenSim::Geometry> newGeometry)
    {
        newGeometry->set_scale_factors(oldGeometry.get_scale_factors());
        newGeometry->set_Appearance(oldGeometry.get_Appearance());
        newGeometry->connectSocket_frame(oldGeometry.getConnectee("frame"));
        newGeometry->setName(oldGeometry.getName());
        OpenSim::Component* owner = UpdOwner(model, oldGeometry);
        OSC_ASSERT_ALWAYS(owner && "the mesh being replaced has no owner? cannot overwrite a root component");
        OSC_ASSERT_ALWAYS(TryDeleteComponentFromModel(model, oldGeometry) && "cannot delete old mesh from model during warping");
        InitializeModel(model);
        InitializeState(model);
        // TODO/HACK: prefer `<attachedGeometry>` block when overwriting meshes defined
        // in frames, because we don't have a way to delete things from the generic
        // component list (yet) #1003
        if (auto* fr = dynamic_cast<OpenSim::Frame*>(owner)) {
            fr->attachGeometry(newGeometry.release());
        }
        else {
            owner->addComponent(newGeometry.release());
        }

        FinalizeConnections(model);
    }

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
    // model warping pipeline, in order to improve performance.
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
            const TPSCoefficients3D& coefficients = lookupTPSCoefficients(tpsInputs);

            // Convert the input mesh into an OSC mesh, so that it's suitable for warping.
            Mesh mesh = ToOscMesh(model, state, inputMesh);

            // Figure out the transform between the input mesh and the landmarks frame
            const Transform mesh2landmarks = to<Transform>(inputMesh.getFrame().findTransformBetween(state, landmarksFrame));

            // Warp the verticies in-place.
            auto vertices = mesh.vertices();
            for (auto& vertex : vertices) {
                vertex = transform_point(mesh2landmarks, vertex);  // put vertex into landmark frame
            }
            ApplyThinPlateWarpToPointsInPlace(coefficients, vertices, static_cast<float>(tpsInputs.blendingFactor));
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
            const TPSCoefficients3D& coefficients = lookupTPSCoefficients(tpsInputs);
            const SimTK::Transform stationParentToLandmarksXform = landmarksFrame.getTransformInGround(state).invert() * parentFrame.getTransformInGround(state);
            const SimTK::Vec3 inputLocationInLandmarksFrame = stationParentToLandmarksXform * locationInParent;
            const auto warpedLocationInLandmarksFrame = to<SimTK::Vec3>(EvaluateTPSEquation(coefficients, to<Vec3>(inputLocationInLandmarksFrame), static_cast<float>(tpsInputs.blendingFactor)));
            const SimTK::Vec3 warpedLocationInStationParentFrame = stationParentToLandmarksXform.invert() * warpedLocationInLandmarksFrame;
            return warpedLocationInStationParentFrame;
        }

        SimTK::Transform lookupTPSAffineTransformWithoutScaling(
            const ThinPlateSplineCommonInputs& tpsInputs)
        {
            OSC_ASSERT_ALWAYS(tpsInputs.applyAffineRotation && "affine rotation must be requested in order to figure out the transform");
            OSC_ASSERT_ALWAYS(tpsInputs.applyAffineTranslation && "affine translation must be requested in order to figure out the transform");
            const TPSCoefficients3D& coefficients = lookupTPSCoefficients(tpsInputs);

            const Vec3d x{normalize(coefficients.a2)};
            const Vec3d y{normalize(coefficients.a3)};
            const Vec3d z{normalize(coefficients.a4)};
            const SimTK::Mat33 rotationMatrix{
                x[0], y[0], z[0],
                x[1], y[1], z[1],
                x[2], y[2], z[2],
            };
            const SimTK::Rotation rotation{rotationMatrix};
            const SimTK::Vec3 translation = to<SimTK::Vec3>(coefficients.a1);
            return SimTK::Transform{rotation, translation};
        }
    private:

        const TPSCoefficients3D& lookupTPSCoefficients(const ThinPlateSplineCommonInputs& tpsInputs)
        {
            // Read source+destination landmark files into independent collections
            const auto sourceLandmarks = lm::ReadLandmarksFromCSVIntoVectorOrThrow(tpsInputs.sourceLandmarksPath);
            const auto destinationLandmarks = lm::ReadLandmarksFromCSVIntoVectorOrThrow(tpsInputs.destinationLandmarksPath);

            // Pair the source+destination landmarks together into a TPS coefficient solver's inputs
            TPSCoefficientSolverInputs3D inputs;
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

            // Solve the coefficients
            m_CoefficientsTODO = CalcCoefficients(inputs);

            // If required, modify the coefficients
            if (not tpsInputs.applyAffineTranslation) {
                m_CoefficientsTODO.a1 = {};
            }
            if (not tpsInputs.applyAffineScale) {
                m_CoefficientsTODO.a2 = normalize(m_CoefficientsTODO.a2);
                m_CoefficientsTODO.a3 = normalize(m_CoefficientsTODO.a3);
                m_CoefficientsTODO.a4 = normalize(m_CoefficientsTODO.a4);
            }
            if (not tpsInputs.applyAffineRotation) {
                m_CoefficientsTODO.a2 = {length(m_CoefficientsTODO.a2), 0.0f, 0.0f};
                m_CoefficientsTODO.a3 = {0.0f, length(m_CoefficientsTODO.a3), 0.0f};
                m_CoefficientsTODO.a4 = {0.0f, 0.0f, length(m_CoefficientsTODO.a4)};
            }
            if (not tpsInputs.applyNonAffineWarp) {
                m_CoefficientsTODO.nonAffineTerms.clear();
            }

            return m_CoefficientsTODO;
        }

        TPSCoefficients3D m_CoefficientsTODO;
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
                msg << get_landmarks_frame() << ": Cannot find this frame in the source model";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
            }

            return messages;
        }

        // Returns TPS `ScalingStep` parameters that are common for all uses of the TPS
        // algorithm.
        ThinPlateSplineScalingStepCommonParams calcTPSScalingStepCommonParams(
            const ScalingParameters& parameters,
            const OpenSim::Model& sourceModel) const
        {
            // Lookup/validate warping inputs.
            const std::optional<std::filesystem::path> modelFilesystemLocation = TryFindInputFile(sourceModel);
            OSC_ASSERT_ALWAYS(modelFilesystemLocation && "The source model has no filesystem location");

            OSC_ASSERT_ALWAYS(not get_source_landmarks_file().empty());
            const std::filesystem::path sourceLandmarksPath = modelFilesystemLocation->parent_path() / get_source_landmarks_file();

            OSC_ASSERT_ALWAYS(not get_destination_landmarks_file().empty());
            const std::filesystem::path destinationLandmarksPath = modelFilesystemLocation->parent_path() / get_destination_landmarks_file();

            OSC_ASSERT_ALWAYS(not get_landmarks_frame().empty());
            const auto* landmarksFrame = FindComponent<OpenSim::Frame>(sourceModel, get_landmarks_frame());
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
            const OpenSim::Model& sourceModel) const
        {
            auto rv = ThinPlateSplineScalingStep::calcTPSScalingStepCommonParams(parameters, sourceModel);
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
                    msg << get_meshes(i) << ": Cannot find this mesh in the source model";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
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
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel);

            // Warp each mesh specified by the `meshes` property.
            for (int i = 0; i < getProperty_meshes().size(); ++i) {
                // Find the mesh in the source model and use it produce the warped mesh.
                const auto* mesh = FindComponent<OpenSim::Mesh>(sourceModel, get_meshes(i));
                OSC_ASSERT_ALWAYS(mesh && "could not find a mesh in the source model");
                std::unique_ptr<InMemoryMesh> warpedMesh = scalingCache.lookupTPSMeshWarp(
                    sourceModel,
                    sourceModel.getWorkingState(),
                    *mesh,
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );
                OSC_ASSERT_ALWAYS(warpedMesh && "warping a mesh in the model failed");

                // Overwrite the mesh in the result model with the warped mesh.
                auto* resultMesh = FindComponentMut<OpenSim::Mesh>(resultModel, get_meshes(i));
                OSC_ASSERT_ALWAYS(resultMesh && "could not find a corresponding mesh in the result model");
                OverwriteGeometry(resultModel, *resultMesh, std::move(warpedMesh));
            }
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
                    msg << get_stations(i) << ": Cannot find this station in the source model";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
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
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel);

            // Warp each station specified by the `stations` property.
            for (int i = 0; i < getProperty_stations().size(); ++i) {
                // Find the station in the source model and use it produce the warped station.
                const auto* station = FindComponent<OpenSim::Station>(sourceModel, get_stations(i));
                OSC_ASSERT_ALWAYS(station && "could not find a station in the source model");

                const SimTK::Vec3 warpedLocation = scalingCache.lookupTPSWarpedRigidPoint(
                    sourceModel,
                    sourceModel.getWorkingState(),
                    station->get_location(),
                    station->getParentFrame(),
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );

                auto* resultStation = FindComponentMut<OpenSim::Station>(resultModel, get_stations(i));
                OSC_ASSERT_ALWAYS(resultStation && "could not find a corresponding station in the result model");
                resultStation->set_location(warpedLocation);
            }
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
                    msg << get_path_points(i) << ": Cannot find this path point in the source model";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
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
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel);

            // Warp each path point specified by the `path_points` property.
            for (int i = 0; i < getProperty_path_points().size(); ++i) {
                // Find the path point in the source model and use it produce the warped path point.
                const auto* pathPoint = FindComponent<OpenSim::PathPoint>(sourceModel, get_path_points(i));
                OSC_ASSERT_ALWAYS(pathPoint && "could not find a path point in the source model");

                const SimTK::Vec3 warpedLocation = scalingCache.lookupTPSWarpedRigidPoint(
                    sourceModel,
                    sourceModel.getWorkingState(),
                    pathPoint->get_location(),
                    pathPoint->getParentFrame(),
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );

                auto* resultPathPoint = FindComponentMut<OpenSim::PathPoint>(resultModel, get_path_points(i));
                OSC_ASSERT_ALWAYS(resultPathPoint && "could not find a corresponding path point in the result model");
                resultPathPoint->set_location(warpedLocation);
            }
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
                    msg << get_offset_frames(i) << ": Cannot find this `PhysicalOffsetFrame` in the source model";
                    messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
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
            // Lookup/validate warping inputs.
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel);

            // Warp each offset frame `translation` specified by the `offset_frames` property.
            for (int i = 0; i < getProperty_offset_frames().size(); ++i) {
                // Find the path point in the source model and use it produce the warped path point.
                const auto* offsetFrame = FindComponent<OpenSim::PhysicalOffsetFrame>(sourceModel, get_offset_frames(i));
                OSC_ASSERT_ALWAYS(offsetFrame && "could not find a `PhysicalOffsetFrame` in the source model");

                const SimTK::Vec3 warpedLocation = scalingCache.lookupTPSWarpedRigidPoint(
                    sourceModel,
                    sourceModel.getWorkingState(),
                    offsetFrame->get_translation(),
                    offsetFrame->getParentFrame(),
                    *commonParams.landmarksFrame,
                    commonParams.tpsInputs
                );

                auto* resultOffsetFrame = FindComponentMut<OpenSim::PhysicalOffsetFrame>(resultModel, get_offset_frames(i));
                OSC_ASSERT_ALWAYS(resultOffsetFrame && "could not find a corresponding `PhysicalOffsetFrame` in the result model");
                resultOffsetFrame->set_translation(warpedLocation);
            }
        }
    };

    // A `ThinPlateSpline` scaling step that substitutes a single new mesh in place of the
    // original one by using the TPS translation + rotation terms to reorient/scale the mesh
    // correctly.
    class ThinPlateSplineMeshSubstitutionScalingStep final : public ThinPlateSplineScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(ThinPlateSplineMeshSubstitutionScalingStep, ThinPlateSplineScalingStep)
    public:
        OpenSim_DECLARE_PROPERTY(source_mesh_component_path, std::string, "TODO");
        OpenSim_DECLARE_PROPERTY(destination_mesh_file, std::string, "TODO");

        explicit ThinPlateSplineMeshSubstitutionScalingStep() :
            ThinPlateSplineScalingStep{"TODO: Substitute Mesh via Inverse Thin-Plate Spline (TPS) Affine Transform"}
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
                msg << get_landmarks_frame() << ": Cannot find `source_mesh_component_path` in the source model";
                messages.emplace_back(ScalingStepValidationState::Error, std::move(msg).str());
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
            const auto commonParams = calcTPSScalingStepCommonParams(parameters, sourceModel);

            // Lookup/validate warping inputs.
            const std::optional<std::filesystem::path> modelFilesystemLocation = TryFindInputFile(sourceModel);
            OSC_ASSERT_ALWAYS(modelFilesystemLocation && "The source model has no filesystem location");

            OSC_ASSERT_ALWAYS(not get_destination_mesh_file().empty());
            const std::filesystem::path destinationMeshPath = modelFilesystemLocation->parent_path() / get_destination_mesh_file();

            OSC_ASSERT_ALWAYS(not get_source_mesh_component_path().empty());
            const auto* sourceMesh = FindComponent<OpenSim::Mesh>(sourceModel, get_source_mesh_component_path());
            OSC_ASSERT_ALWAYS(sourceMesh && "could not find `source_mesh_component_path` in the model");

            // TODO: everything below is a hack because manipulating models
            // via OpenSim is like pulling teeth, underwater, from a shark,
            // with rabies, using your bare hands.

            const SimTK::Transform t = cache.lookupTPSAffineTransformWithoutScaling(commonParams.tpsInputs);
            const SimTK::Vec3 newScaleFactors =
                (commonParams.tpsInputs.destinationLandmarksPrescale/commonParams.tpsInputs.sourceLandmarksPrescale) * sourceMesh->get_scale_factors();

            // Find existing mesh
            auto* destinationMesh = FindComponentMut<OpenSim::Mesh>(resultModel, get_source_mesh_component_path());
            OSC_ASSERT_ALWAYS(destinationMesh && "could not find `source_mesh_component_path` in the result model");
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

            auto* newMesh = new OpenSim::Mesh{};
            newMesh->setName(existingMeshName);
            newMesh->set_scale_factors(newScaleFactors);
            newMesh->set_mesh_file(destinationMeshPath.string());
            newMesh->updSocket("frame").setConnecteePath(newFrame->getAbsolutePathString());
            resultModel.addComponent(newMesh);
        }
    };

    // A `ScalingStep` that scales the masses of bodies in the model.
    class BodyMassesScalingStep final : public ScalingStep {
        OpenSim_DECLARE_CONCRETE_OBJECT(BodyMassesScalingStep, ScalingStep)
    public:
        explicit BodyMassesScalingStep() :
            ScalingStep{"TODO: Scale Body Masses to Subject Mass"}
        {
            setDescription("Scales the masses of bodies in the model to match the subject's mass");
        }

    private:
        void implForEachScalingParameterDeclaration(const std::function<void(const ScalingParameterDeclaration&)>& callback) const final
        {
            callback(ScalingParameterDeclaration{"blending_factor", 1.0});
            callback(ScalingParameterDeclaration{"subject_mass", 75.0});
        }
    };

    // Compile-time `Typelist` containing all `ScalingStep`s that this UI handles.
    using AllScalingStepTypes = Typelist<
        ThinPlateSplineMeshesScalingStep,
        ThinPlateSplineStationsScalingStep,
        ThinPlateSplinePathPointsScalingStep,
        ThinPlateSplineOffsetFrameTranslationScalingStep,
        ThinPlateSplineMeshSubstitutionScalingStep,
        BodyMassesScalingStep
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
        void saveScalingDocumentTo(const std::filesystem::path& p)
        {
            if (scalingDocument->print(p.string())) {
                //scalingDocument->setInlined(false, p.string());
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
            resultModel.finalizeConnections(resultModel);
            resultModel.finalizeFromProperties();

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
    class ModelWarperV3UIState final {
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

                // Create warped geom dir
                const auto warpedGeometryDir = modelFilesystemLocation->parent_path() / "WarpedGeometry";
                if (not std::filesystem::exists(warpedGeometryDir)) {
                    std::filesystem::create_directories(warpedGeometryDir);
                }

                // Export `InMemoryMesh`es to disk (#1003)
                for (const InMemoryMesh& mesh : scaled->getModel().getComponentList<InMemoryMesh>()) {
                    // Figure out output file name.
                    const OpenSim::Mesh& inputMesh = sourceModel.getComponent<OpenSim::Mesh>(mesh.getAbsolutePath());
                    const auto warpedMeshAbsPath = std::filesystem::weakly_canonical(warpedGeometryDir / std::filesystem::path{inputMesh.get_mesh_file()}.filename().replace_extension(".obj"));

                    // Write warped mesh data to disk in an OBJ format.
                    {
                        std::ofstream objStream{warpedMeshAbsPath, std::ios::trunc};
                        objStream.exceptions(std::ios::badbit | std::ios::failbit);
                        write_as_obj(objStream, mesh.getOscMesh(), ObjMetadata{"osc-model-warper"});
                    }

                    // Overwrite the `InMemoryMesh` in `copy`
                    InMemoryMesh& copyMesh = copy->updComponent<InMemoryMesh>(mesh.getAbsolutePath());
                    auto newMesh = std::make_unique<OpenSim::Mesh>();
                    newMesh->set_mesh_file(warpedMeshAbsPath.string());
                    OverwriteGeometry(*copy, copyMesh, std::move(newMesh));
                }
            }

            auto editor = std::make_unique<ModelEditorTab>(parent_, std::move(copy));
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


        // actions
        void actionCreateNewSourceModel()
        {
            m_ScalingState->upd_scratch().resetSourceModel();
            updateScaledModel();
            m_ScalingState->commit_scratch("Create new source model");
        }

        void actionOpenOsimOrPromptUser(std::optional<std::filesystem::path> path)
        {
            if (not path) {
                path = prompt_user_to_select_file({"osim"});
            }

            if (path) {
                App::singleton<RecentFiles>()->push_back(*path);
                m_ScalingState->upd_scratch().loadSourceModelFromOsim(*path);
                updateScaledModel();
                m_ScalingState->commit_scratch("Loaded osim file");
            }
        }

        void actionCreateNewScalingDocument()
        {
            m_ScalingState->upd_scratch().resetScalingDocument();
            updateScaledModel();
            m_ScalingState->commit_scratch("Create new scaling document");
        }

        void actionOpenScalingDocument()
        {
            if (const auto path = prompt_user_to_select_file({"xml"})) {
                m_ScalingState->upd_scratch().loadScalingDocument(*path);
                updateScaledModel();
                m_ScalingState->commit_scratch("Loaded scaling document");
            }
        }

        void actionSaveScalingDocument()
        {
            if (const auto existingPath = m_ScalingState->scratch().scalingDocumentFilesystemLocation()) {
                m_ScalingState->upd_scratch().saveScalingDocumentTo(*existingPath);
            }
            else if (const auto userSelectedPath = prompt_user_for_file_save_location_add_extension_if_necessary("xml")) {
                m_ScalingState->upd_scratch().saveScalingDocumentTo(*userSelectedPath);
            }
            // else: doesn't have an existing filesystem location and the user cancelled the dialog: do nothing
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
            const float contentHeight = static_cast<float>(messages.size() + 2) * ui::get_text_line_height();
            const float regionHeight = ui::get_content_region_available().y;
            const float top = 0.5f * (regionHeight - contentHeight);

            ui::set_cursor_pos({0.0f, top});

            // header line
            {
                std::stringstream ss;
                ss << "Cannot show model: " << messages.size() << " validation error" << (messages.size() > 1 ? "s" : "") << " detected:";
                ui::draw_text_centered(std::move(ss).str());
            }

            // error line(s)
            int id = 0;
            for ([[maybe_unused]] const auto& message : messages) {
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
            const float lineHeight = ui::get_text_line_height();
            constexpr float numLines = 3.0f;
            const float top = 0.5f * (h - numLines*lineHeight);

            ui::set_cursor_pos({0.0f, top});
            ui::draw_text_centered("An error occured while trying to scale the model:");
            ui::draw_text_centered(message);
            if (ui::draw_button_centered(OSC_ICON_RECYCLE " Retry Scaling")) {
                m_State->actionRetryScalingDeferred();
            }
        }

        std::shared_ptr<ModelWarperV3UIState> m_State;
    };

    class WarpedModelExporterPopup final : public StandardPopup {
    public:
        explicit WarpedModelExporterPopup() :
            StandardPopup{"hello, popup world!"}
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
                ui::pop_style_color();
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
        UndoButton m_UndoButton{this, m_State->getUndoRedoPtr()};
        RedoButton m_RedoButton{this, m_State->getUndoRedoPtr()};
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
            ui::draw_dummy({0.0f, 0.75f*ui::get_text_line_height()});
            draw_scaling_steps();
        }

        void draw_scaling_parameters()
        {
            ui::draw_text_centered("Scaling Parameters");
            ui::draw_separator();
            ui::draw_dummy({0.0f, 0.5f*ui::get_text_line_height()});
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
            ui::draw_dummy({0.0f, 0.5f*ui::get_text_line_height()});

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

            ui::draw_dummy({0.0f, 0.25f*ui::get_text_line_height()});
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

                const Vec2 oldCursorPos = ui::get_cursor_pos();
                const float endX = oldCursorPos.x + ui::get_content_region_available().x;

                const Vec2 newCursorPos = {endX - ui::calc_button_size(deletionButtonIcon).x, oldCursorPos.y};
                ui::set_cursor_pos(newCursorPos);
                if (ui::draw_small_button(deletionButtonIcon)) {
                    m_State->eraseScalingStepDeferred(step);
                }
            }

            // draw validation messages
            {
                const auto messages = m_State->validateStep(step);
                if (not messages.empty()) {
                    ui::draw_dummy({0.0f, 0.2f * ui::get_text_line_height()});
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
                    ui::draw_dummy({0.0f, 0.2f * ui::get_text_line_height()});
                }
            }

            // draw property editors
            ui::indent(1.0f*ui::get_text_line_height());
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
            ui::unindent(1.0f*ui::get_text_line_height());
            ui::draw_dummy({0.0f, 0.5f*ui::get_text_line_height()});
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

class osc::ModelWarperV3Tab::Impl final : public TabPrivate {
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
            ui::enable_dockspace_over_main_viewport();
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

CStringView osc::ModelWarperV3Tab::id() { return Impl::static_label(); }

osc::ModelWarperV3Tab::ModelWarperV3Tab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::ModelWarperV3Tab::impl_on_mount() { private_data().on_mount(); }
void osc::ModelWarperV3Tab::impl_on_unmount() { private_data().on_unmount(); }
void osc::ModelWarperV3Tab::impl_on_tick() { private_data().on_tick(); }
void osc::ModelWarperV3Tab::impl_on_draw_main_menu() { private_data().on_draw_main_menu(); }
void osc::ModelWarperV3Tab::impl_on_draw() { private_data().on_draw(); }

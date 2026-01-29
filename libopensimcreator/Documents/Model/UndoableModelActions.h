#pragma once

#include <libopynsim/documents/landmarks/named_landmark.h>
#include <liboscar/maths/euler_angles.h>
#include <liboscar/maths/vector3.h>

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace OpenSim { class ContactGeometry; }
namespace OpenSim { class Coordinate; }
namespace OpenSim { class Geometry; }
namespace OpenSim { class GeometryPath; }
namespace OpenSim { class Joint; }
namespace OpenSim { class Marker; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class Object; }
namespace OpenSim { class PathPoint; }
namespace OpenSim { class PhysicalFrame; }
namespace OpenSim { class PhysicalOffsetFrame; }
namespace OpenSim { class Scholz2015GeometryPathObstacle; }
namespace OpenSim { class Station; }
namespace OpenSim { class WrapObject; }
namespace opyn { class ModelStatePair; }
namespace osc { class ModelStatePairWithSharedEnvironment; }
namespace osc { class ObjectPropertyEdit; }
namespace osc { class SceneCache; }
namespace osc { class UndoableModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    // prompt the user for a save location and then save the model to the specified location
    void ActionSaveCurrentModelAs(
        const std::shared_ptr<opyn::ModelStatePair>&
    );

    // create a new model and show it in a new tab
    void ActionNewModel(
        Widget& parent
    );

    // prompt a user to open a model file and open it in a new tab
    void ActionOpenModel(
        Widget&
    );

    // open the specified model in a loading tab
    void ActionOpenModel(
        Widget&,
        const std::filesystem::path&
    );

    // Tries to save the given model, potentially asynchronously, as an on-disk file
    // then calls `callback` with `true` if the save was successful; otherwise, calls it
    // with `false`.
    void ActionSaveModelAsync(
        const std::shared_ptr<opyn::ModelStatePair>&,
        std::function<void(bool)> callback = [](bool){}
    );

    // try to delete an undoable-model's current selection
    //
    // "try", because some things are difficult to delete from OpenSim models
    void ActionTryDeleteSelectionFromEditedModel(
        opyn::ModelStatePair&
    );

    // disable all wrapping surfaces in the current model
    void ActionDisableAllWrappingSurfaces(
        opyn::ModelStatePair&
    );

    // enable all wrapping surfaces in the current model
    void ActionEnableAllWrappingSurfaces(
        opyn::ModelStatePair&
    );

    // loads an STO file against the current model and opens it in a new tab
    bool ActionLoadSTOFileAgainstModel(
        Widget&,
        const ModelStatePairWithSharedEnvironment&,
        const std::filesystem::path& stoPath
    );

    // start simulating the given model in a forward-dynamic simulator tab
    bool ActionStartSimulatingModel(
        Widget&,
        const ModelStatePairWithSharedEnvironment&
    );

    // reload the given model from its backing file (if applicable)
    bool ActionUpdateModelFromBackingFile(
        UndoableModelStatePair&
    );

    // copies the full absolute path to the osim to the clipboard
    bool ActionCopyModelPathToClipboard(
        const opyn::ModelStatePair&
    );

    // try to automatically set the model's scale factor based on how big the scene is
    bool ActionAutoscaleSceneScaleFactor(
        opyn::ModelStatePair&
    );

    // toggle model frame visibility
    bool ActionToggleFrames(
        opyn::ModelStatePair&
    );

    // toggle model marker visibility
    bool ActionToggleMarkers(
        opyn::ModelStatePair&
    );

    // toggle contact geometry visibility
    bool ActionToggleContactGeometry(
        opyn::ModelStatePair&
    );

    // toggle force visibility (#887)
    bool ActionToggleForces(
        opyn::ModelStatePair&
    );

    // toggle model wrap geometry visibility
    bool ActionToggleWrapGeometry(
        opyn::ModelStatePair&
    );

    // open the parent directory of the model's backing file (if applicable) in an OS file explorer window
    bool ActionOpenOsimParentDirectory(const OpenSim::Model&);

    // open the model's backing file (if applicable) in an OS-determined default for osims
    bool ActionOpenOsimInExternalEditor(const OpenSim::Model&);

    // force a reload of the model, and its associated assets, from its backing file
    bool ActionReloadOsimFromDisk(
        UndoableModelStatePair&,
        SceneCache&
    );

    // add an offset frame to the current selection (if applicable)
    bool ActionAddOffsetFrameToPhysicalFrame(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& physFramePath
    );

    // returns true if the specified joint (if valid) can be re-zeroed
    bool CanRezeroJoint(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& jointPath
    );

    // re-zeroes the selected joint (if applicable)
    bool ActionRezeroJoint(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& jointPath
    );

    // adds a parent offset frame to the selected joint (if applicable)
    bool ActionAddParentOffsetFrameToJoint(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& jointPath
    );

    // adds a child offset frame to the selected joint (if applicable)
    bool ActionAddChildOffsetFrameToJoint(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& jointPath
    );

    // sets the name of the selected component (if applicable)
    bool ActionSetComponentName(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& componentPath,
        const std::string&
    );

    // changes the type of the selected joint (if applicable) to the provided joint
    bool ActionChangeJointTypeTo(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& jointPath,
        std::unique_ptr<OpenSim::Joint>
    );

    // attaches geometry to the selected physical frame (if applicable)
    bool ActionAttachGeometryToPhysicalFrame(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& physFramePath,
        std::unique_ptr<OpenSim::Geometry>
    );

    // assigns contact geometry to the selected HCF (if applicable)
    bool ActionAssignContactGeometryToHCF(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& hcfPath,
        const OpenSim::ComponentPath& contactGeomPath
    );

    // applies a property edit to the model
    bool ActionApplyPropertyEdit(
        opyn::ModelStatePair&,
        ObjectPropertyEdit&
    );

    // adds a path point to the selected geometry path
    bool ActionAddPathPointToGeometryPath(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& geometryPathPath,
        const OpenSim::ComponentPath& pointPhysFrame
    );

    // adds a path point to the selected path actuator (if applicable)
    bool ActionAddPathPointToPathActuator(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& pathActuatorPath,
        const OpenSim::ComponentPath& pointPhysFrame
    );

    // flags for reassignment
    enum class SocketReassignmentFlags {
        None                                 = 0,
        TryReexpressComponentInNewConnectee  = 1<<0,
    };
    constexpr bool operator&(SocketReassignmentFlags lhs, SocketReassignmentFlags rhs)
    {
        return std::to_underlying(lhs) & std::to_underlying(rhs);
    }

    // attempts to reassign a component's socket connection (returns false and writes to `error` on failure)
    bool ActionReassignComponentSocket(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& componentAbsPath,
        const std::string& socketName,
        const OpenSim::Object& connectee,
        SocketReassignmentFlags flags,
        std::string& error
    );

    // details of a body that should be added to a model
    struct BodyDetails final {

        BodyDetails();

        Vector3 centerOfMass;
        Vector3 inertia;
        float mass;
        std::string parentFrameAbsPath;
        std::string bodyName;
        size_t jointTypeIndex;
        std::string jointName;
        std::unique_ptr<OpenSim::Geometry> maybeGeometry;
        bool addOffsetFrames;
    };

    // add a new body to the model
    bool ActionAddBodyToModel(opyn::ModelStatePair&, const BodyDetails&);

    // add the given component into the model graph, or throw
    bool ActionAddComponentToModel(opyn::ModelStatePair&, std::unique_ptr<OpenSim::Component>);
    bool ActionAddComponentToModel(opyn::ModelStatePair&, std::unique_ptr<OpenSim::Component>, const OpenSim::ComponentPath& desiredParent);

    // add the given `OpenSim::WrapObject` to the `WrapObjectSet` of
    // the `OpenSim::PhysicalFrame` located at `physicalFramePath`
    bool ActionAddWrapObjectToPhysicalFrame(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath& physicalFramePath,
        std::unique_ptr<OpenSim::WrapObject> wrapObjPtr
    );

    // add the given `OpenSim::WrapObject` to the `OpenSim::GeometryPath`'s
    // wrap object set, which makes the path wrap around the wrap object
    bool ActionAddWrapObjectToGeometryPathWraps(
        opyn::ModelStatePair&,
        const OpenSim::GeometryPath&,
        const OpenSim::WrapObject&
    );

    // remove the given `OpenSim::WrapObject` from the `OpenSim::GeometryPath`'s
    // wrap object set
    //
    // does nothing if the `OpenSim::WrapObject` isn't in the path's wrap set
    bool ActionRemoveWrapObjectFromGeometryPathWraps(
        opyn::ModelStatePair&,
        const OpenSim::GeometryPath&,
        const OpenSim::WrapObject&
    );

    // Zeroes all `OpenSim::Coordinates` in the model. If a coordinate is clamped
    // then this tries to get as close to zero as possible while obeying the clamp
    bool ActionZeroAllCoordinates(
        opyn::ModelStatePair&
    );

    // set the speed of a coordinate
    bool ActionSetCoordinateSpeed(
        opyn::ModelStatePair&,
        const OpenSim::Coordinate&,
        double newSpeed
    );

    // set the speed of a coordinate and ensure it is saved
    bool ActionSetCoordinateSpeedAndSave(
        opyn::ModelStatePair&,
        const OpenSim::Coordinate&,
        double newSpeed
    );

    // set a coordinate (un)locked
    bool ActionSetCoordinateLockedAndSave(
        opyn::ModelStatePair&,
        const OpenSim::Coordinate&,
        bool
    );

    // set the value of a coordinate
    bool ActionSetCoordinateValue(
        opyn::ModelStatePair&,
        const OpenSim::Coordinate&,
        double newValue
    );

    // set the value of a coordinate and ensure it is saved
    bool ActionSetCoordinateValueAndSave(
        opyn::ModelStatePair&,
        const OpenSim::Coordinate&,
        double newValue
    );

    // sets the `Appearance` property of the pointed-to component, and all its children, to have visible = bool
    bool ActionSetComponentAndAllChildrensIsVisibleTo(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath&,
        bool newVisibility
    );

    // sets the `Appearance` property of all components in the model to `visible = false`, followed by setting the
    // `Appearance` property of the pointed-to component, and all its children, to `visible = true`
    bool ActionShowOnlyComponentAndAllChildren(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath&
    );

    // sets the `Appearance` property of all components in the model to `visible = visible` if that component has
    // the given concrete class name
    bool ActionSetComponentAndAllChildrenWithGivenConcreteClassNameIsVisibleTo(
        opyn::ModelStatePair&,
        const OpenSim::ComponentPath&,
        std::string_view concreteClassName,
        bool newVisibility
    );

    // sets the location of the given station in its parent frame to its old location plus the provided delta
    //
    // (does not save the change to undo/redo storage)
    bool ActionTranslateStation(
        opyn::ModelStatePair&,
        const OpenSim::Station&,
        const Vector3& deltaPosition
    );

    // sets the location of the given station in its parent frame to its old location plus the provided vector
    //
    // (saves the change to undo/redo storage)
    bool ActionTranslateStationAndSave(
        opyn::ModelStatePair&,
        const OpenSim::Station&,
        const Vector3& deltaPosition
    );

    // sets the location of the given path point in its parent frame to its old location plus the provided delta
    //
    // (does not save the change to undo/redo storage)
    bool ActionTranslatePathPoint(
        opyn::ModelStatePair&,
        const OpenSim::PathPoint&,
        const Vector3& deltaPosition
    );

    // sets the location of the given path point in its parent frame to its old location plus the provided delta
    //
    // (saves the change to undo/redo storage)
    bool ActionTranslatePathPointAndSave(
        opyn::ModelStatePair&,
        const OpenSim::PathPoint&,
        const Vector3& deltaPosition
    );

    bool ActionTransformPofV2(
        opyn::ModelStatePair&,
        const OpenSim::PhysicalOffsetFrame&,
        const Vector3& newTranslation,
        const EulerAngles& newEulers
    );
    bool ActionTransformWrapObject(
        opyn::ModelStatePair&,
        const OpenSim::WrapObject&,
        const Vector3& deltaPosition,
        const EulerAngles& newEulers
    );
    bool ActionTransformContactGeometry(
        opyn::ModelStatePair&,
        const OpenSim::ContactGeometry&,
        const Vector3& deltaPosition,
        const EulerAngles& newEulers
    );
    bool ActionFitSphereToMesh(opyn::ModelStatePair&, const OpenSim::Mesh&);
    bool ActionFitEllipsoidToMesh(opyn::ModelStatePair&, const OpenSim::Mesh&);
    bool ActionFitPlaneToMesh(opyn::ModelStatePair&, const OpenSim::Mesh&);
    bool ActionImportLandmarks(
        opyn::ModelStatePair&,
        std::span<const lm::NamedLandmark>,
        std::optional<std::string> maybeName,
        std::optional<std::string> maybeTargetFrameAbsPath
    );
    void ActionExportModelGraphToDotviz(const std::shared_ptr<opyn::ModelStatePair>&);
    bool ActionExportModelGraphToDotvizClipboard(const OpenSim::Model&);
    bool ActionExportModelMultibodySystemAsDotviz(const OpenSim::Model&);
    bool ActionBakeStationDefinedFrames(opyn::ModelStatePair&);
    bool ActionMoveMarkerToModelMarkerSet(opyn::ModelStatePair&, const OpenSim::Marker&);

    bool ActionTranslateContactHint(opyn::ModelStatePair&, const OpenSim::Scholz2015GeometryPathObstacle&, const Vector3& deltaPosition);
    bool ActionTranslateContactHintAndSave(opyn::ModelStatePair&, const OpenSim::Scholz2015GeometryPathObstacle&, const Vector3& deltaPosition);
}

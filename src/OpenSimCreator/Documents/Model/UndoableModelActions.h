#pragma once

#include <OpenSimCreator/Documents/Landmarks/NamedLandmark.h>

#include <oscar/Maths/EulerAngles.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Shims/Cpp23/utility.h>
#include <oscar/Utils/ParentPtr.h>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace OpenSim { class ContactGeometry; }
namespace OpenSim { class Coordinate; }
namespace OpenSim { class Geometry; }
namespace OpenSim { class GeometryPath; }
namespace OpenSim { class Joint; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class Model; }
namespace OpenSim { class Object; }
namespace OpenSim { class PathPoint; }
namespace OpenSim { class PhysicalFrame; }
namespace OpenSim { class PhysicalOffsetFrame; }
namespace OpenSim { class Station; }
namespace OpenSim { class WrapObject; }
namespace osc { class IMainUIStateAPI; }
namespace osc { class IModelStatePair; }
namespace osc { class ObjectPropertyEdit; }
namespace osc { class SceneCache; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    // prompt the user for a save location and then save the model to the specified location
    void ActionSaveCurrentModelAs(
        UndoableModelStatePair&
    );

    // create a new model and show it in a new tab
    void ActionNewModel(
        const ParentPtr<IMainUIStateAPI>&
    );

    // prompt a user to open a model file and open it in a new tab
    void ActionOpenModel(
        const ParentPtr<IMainUIStateAPI>&
    );

    // open the specified model in a loading tab
    void ActionOpenModel(
        const ParentPtr<IMainUIStateAPI>&,
        const std::filesystem::path&
    );

    // try to save the given model file to disk
    bool ActionSaveModel(
        IMainUIStateAPI&,
        UndoableModelStatePair&
    );

    // try to delete an undoable-model's current selection
    //
    // "try", because some things are difficult to delete from OpenSim models
    void ActionTryDeleteSelectionFromEditedModel(
        IModelStatePair&
    );

    // disable all wrapping surfaces in the current model
    void ActionDisableAllWrappingSurfaces(
        IModelStatePair&
    );

    // enable all wrapping surfaces in the current model
    void ActionEnableAllWrappingSurfaces(
        IModelStatePair&
    );

    // loads an STO file against the current model and opens it in a new tab
    bool ActionLoadSTOFileAgainstModel(
        const ParentPtr<IMainUIStateAPI>&,
        const IModelStatePair&,
        const std::filesystem::path& stoPath
    );

    // start simulating the given model in a forward-dynamic simulator tab
    bool ActionStartSimulatingModel(
        const ParentPtr<IMainUIStateAPI>&,
        const IModelStatePair&
    );

    // reload the given model from its backing file (if applicable)
    bool ActionUpdateModelFromBackingFile(
        UndoableModelStatePair&
    );

    // copies the full absolute path to the osim to the clipboard
    bool ActionCopyModelPathToClipboard(
        const UndoableModelStatePair&
    );

    // try to automatically set the model's scale factor based on how big the scene is
    bool ActionAutoscaleSceneScaleFactor(
        IModelStatePair&
    );

    // toggle model frame visibility
    bool ActionToggleFrames(
        IModelStatePair&
    );

    // toggle model marker visibility
    bool ActionToggleMarkers(
        IModelStatePair&
    );

    // toggle contact geometry visibility
    bool ActionToggleContactGeometry(
        IModelStatePair&
    );

    // toggle force visibility (#887)
    bool ActionToggleForces(
        IModelStatePair&
    );

    // toggle model wrap geometry visibility
    bool ActionToggleWrapGeometry(
        IModelStatePair&
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
        IModelStatePair&,
        const OpenSim::ComponentPath& physFramePath
    );

    // returns true if the specified joint (if valid) can be re-zeroed
    bool CanRezeroJoint(
        IModelStatePair&,
        const OpenSim::ComponentPath& jointPath
    );

    // re-zeroes the selected joint (if applicable)
    bool ActionRezeroJoint(
        IModelStatePair&,
        const OpenSim::ComponentPath& jointPath
    );

    // adds a parent offset frame to the selected joint (if applicable)
    bool ActionAddParentOffsetFrameToJoint(
        IModelStatePair&,
        const OpenSim::ComponentPath& jointPath
    );

    // adds a child offset frame to the selected joint (if applicable)
    bool ActionAddChildOffsetFrameToJoint(
        IModelStatePair&,
        const OpenSim::ComponentPath& jointPath
    );

    // sets the name of the selected component (if applicable)
    bool ActionSetComponentName(
        IModelStatePair&,
        const OpenSim::ComponentPath& componentPath,
        const std::string&
    );

    // changes the type of the selected joint (if applicable) to the provided joint
    bool ActionChangeJointTypeTo(
        IModelStatePair&,
        const OpenSim::ComponentPath& jointPath,
        std::unique_ptr<OpenSim::Joint>
    );

    // attaches geometry to the selected physical frame (if applicable)
    bool ActionAttachGeometryToPhysicalFrame(
        IModelStatePair&,
        const OpenSim::ComponentPath& physFramePath,
        std::unique_ptr<OpenSim::Geometry>
    );

    // assigns contact geometry to the selected HCF (if applicable)
    bool ActionAssignContactGeometryToHCF(
        IModelStatePair&,
        const OpenSim::ComponentPath& hcfPath,
        const OpenSim::ComponentPath& contactGeomPath
    );

    // applies a property edit to the model
    bool ActionApplyPropertyEdit(
        IModelStatePair&,
        ObjectPropertyEdit&
    );

    // adds a path point to the selected path actuator (if applicable)
    bool ActionAddPathPointToPathActuator(
        IModelStatePair&,
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
        return cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs);
    }

    // attempts to reassign a component's socket connection (returns false and writes to `error` on failure)
    bool ActionReassignComponentSocket(
        IModelStatePair&,
        const OpenSim::ComponentPath& componentAbsPath,
        const std::string& socketName,
        const OpenSim::Object& connectee,
        SocketReassignmentFlags flags,
        std::string& error
    );

    // details of a body that should be added to a model
    struct BodyDetails final {

        BodyDetails();

        Vec3 centerOfMass;
        Vec3 inertia;
        float mass;
        std::string parentFrameAbsPath;
        std::string bodyName;
        size_t jointTypeIndex;
        std::string jointName;
        std::unique_ptr<OpenSim::Geometry> maybeGeometry;
        bool addOffsetFrames;
    };

    // add a new body to the model
    bool ActionAddBodyToModel(IModelStatePair&, const BodyDetails&);

    // add the given component into the model graph, or throw
    bool ActionAddComponentToModel(IModelStatePair&, std::unique_ptr<OpenSim::Component>);

    // add the given `OpenSim::WrapObject` to the `WrapObjectSet` of
    // the `OpenSim::PhysicalFrame` located at `physicalFramePath`
    bool ActionAddWrapObjectToPhysicalFrame(
        IModelStatePair&,
        const OpenSim::ComponentPath& physicalFramePath,
        std::unique_ptr<OpenSim::WrapObject> wrapObjPtr
    );

    // add the given `OpenSim::WrapObject` to the `OpenSim::GeometryPath`'s
    // wrap object set, which makes the path wrap around the wrap object
    bool ActionAddWrapObjectToGeometryPathWraps(
        IModelStatePair&,
        const OpenSim::GeometryPath&,
        const OpenSim::WrapObject&
    );

    // remove the given `OpenSim::WrapObject` from the `OpenSim::GeometryPath`'s
    // wrap object set
    //
    // does nothing if the `OpenSim::WrapObject` isn't in the path's wrap set
    bool ActionRemoveWrapObjectFromGeometryPathWraps(
        IModelStatePair&,
        const OpenSim::GeometryPath&,
        const OpenSim::WrapObject&
    );

    // set the speed of a coordinate
    bool ActionSetCoordinateSpeed(
        IModelStatePair&,
        const OpenSim::Coordinate&,
        double newSpeed
    );

    // set the speed of a coordinate and ensure it is saved
    bool ActionSetCoordinateSpeedAndSave(
        IModelStatePair&,
        const OpenSim::Coordinate&,
        double newSpeed
    );

    // set a coordinate (un)locked
    bool ActionSetCoordinateLockedAndSave(
        IModelStatePair&,
        const OpenSim::Coordinate&,
        bool
    );

    // set the value of a coordinate
    bool ActionSetCoordinateValue(
        IModelStatePair&,
        const OpenSim::Coordinate&,
        double newValue
    );

    // set the value of a coordinate and ensure it is saved
    bool ActionSetCoordinateValueAndSave(
        IModelStatePair&,
        const OpenSim::Coordinate&,
        double newValue
    );

    // sets the `Appearance` property of the pointed-to component, and all its children, to have visible = bool
    bool ActionSetComponentAndAllChildrensIsVisibleTo(
        IModelStatePair&,
        const OpenSim::ComponentPath&,
        bool newVisibility
    );

    // sets the `Appearance` property of all components in the model to `visible = false`, followed by setting the
    // `Appearance` property of the pointed-to component, and all its children, to `visible = true`
    bool ActionShowOnlyComponentAndAllChildren(
        IModelStatePair&,
        const OpenSim::ComponentPath&
    );

    // sets the `Appearance` property of all components in the model to `visible = visible` if that component has
    // the given concrete class name
    bool ActionSetComponentAndAllChildrenWithGivenConcreteClassNameIsVisibleTo(
        IModelStatePair&,
        const OpenSim::ComponentPath&,
        std::string_view concreteClassName,
        bool newVisibility
    );

    // sets the location of the given station in its parent frame to its old location plus the provided delta
    //
    // (does not save the change to undo/redo storage)
    bool ActionTranslateStation(
        IModelStatePair&,
        const OpenSim::Station&,
        const Vec3& deltaPosition
    );

    // sets the location of the given station in its parent frame to its old location plus the provided vector
    //
    // (saves the change to undo/redo storage)
    bool ActionTranslateStationAndSave(
        IModelStatePair&,
        const OpenSim::Station&,
        const Vec3& deltaPosition
    );

    // sets the location of the given path point in its parent frame to its old location plus the provided delta
    //
    // (does not save the change to undo/redo storage)
    bool ActionTranslatePathPoint(
        IModelStatePair&,
        const OpenSim::PathPoint&,
        const Vec3& deltaPosition
    );

    // sets the location of the given path point in its parent frame to its old location plus the provided delta
    //
    // (saves the change to undo/redo storage)
    bool ActionTranslatePathPointAndSave(
        IModelStatePair&,
        const OpenSim::PathPoint&,
        const Vec3& deltaPosition
    );

    bool ActionTransformPofV2(
        IModelStatePair&,
        const OpenSim::PhysicalOffsetFrame&,
        const Vec3& newTranslation,
        const EulerAngles& newEulers
    );
    bool ActionTransformWrapObject(
        IModelStatePair&,
        const OpenSim::WrapObject&,
        const Vec3& deltaPosition,
        const EulerAngles& newEulers
    );
    bool ActionTransformContactGeometry(
        IModelStatePair&,
        const OpenSim::ContactGeometry&,
        const Vec3& deltaPosition,
        const EulerAngles& newEulers
    );
    bool ActionFitSphereToMesh(IModelStatePair&, const OpenSim::Mesh&);
    bool ActionFitEllipsoidToMesh(IModelStatePair&, const OpenSim::Mesh&);
    bool ActionFitPlaneToMesh(IModelStatePair&, const OpenSim::Mesh&);
    bool ActionImportLandmarks(IModelStatePair&, std::span<const lm::NamedLandmark>, std::optional<std::string> maybeName);
    bool ActionExportModelGraphToDotviz(const OpenSim::Model&);
    bool ActionExportModelGraphToDotvizClipboard(const OpenSim::Model&);
    bool ActionExportModelMultibodySystemAsDotviz(const OpenSim::Model&);
}

#pragma once

#include <oscar/Maths/Vec3.hpp>
#include <oscar/Shims/Cpp23/utility.hpp>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace OpenSim { class ContactGeometry; }
namespace OpenSim { class Coordinate; }
namespace OpenSim { class Geometry; }
namespace OpenSim { class Joint; }
namespace OpenSim { class Object; }
namespace OpenSim { class Mesh; }
namespace OpenSim { class PathPoint; }
namespace OpenSim { class PhysicalFrame; }
namespace OpenSim { class PhysicalOffsetFrame; }
namespace OpenSim { class Station; }
namespace OpenSim { class WrapObject; }
namespace osc { class MainUIStateAPI; }
namespace osc { class ObjectPropertyEdit; }
namespace osc { template<typename T> class ParentPtr; }
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
        ParentPtr<MainUIStateAPI> const&
    );

    // prompt a user to open a model file and open it in a new tab
    void ActionOpenModel(
        ParentPtr<MainUIStateAPI> const&
    );

    // open the specified model in a loading tab
    void ActionOpenModel(
        ParentPtr<MainUIStateAPI> const&,
        std::filesystem::path const&
    );

    // try to save the given model file to disk
    bool ActionSaveModel(
        MainUIStateAPI&,
        UndoableModelStatePair&
    );

    // try to delete an undoable-model's current selection
    //
    // "try", because some things are difficult to delete from OpenSim models
    void ActionTryDeleteSelectionFromEditedModel(
        UndoableModelStatePair&
    );

    // try to undo currently edited model to earlier state
    void ActionUndoCurrentlyEditedModel(
        UndoableModelStatePair&
    );

    // try to redo currently edited model to later state
    void ActionRedoCurrentlyEditedModel(
        UndoableModelStatePair&
    );

    // disable all wrapping surfaces in the current model
    void ActionDisableAllWrappingSurfaces(
        UndoableModelStatePair&
    );

    // enable all wrapping surfaces in the current model
    void ActionEnableAllWrappingSurfaces(
        UndoableModelStatePair&
    );

    // clears the current selection in the model
    void ActionClearSelectionFromEditedModel(
        UndoableModelStatePair&
    );

    // loads an STO file against the current model and opens it in a new tab
    bool ActionLoadSTOFileAgainstModel(
        ParentPtr<MainUIStateAPI> const&,
        UndoableModelStatePair const&,
        std::filesystem::path const& stoPath
    );

    // start simulating the given model in a forward-dynamic simulator tab
    bool ActionStartSimulatingModel(
        ParentPtr<MainUIStateAPI> const&,
        UndoableModelStatePair const&
    );

    // reload the given model from its backing file (if applicable)
    bool ActionUpdateModelFromBackingFile(
        UndoableModelStatePair&
    );

    // copies the full absolute path to the osim to the clipboard
    bool ActionCopyModelPathToClipboard(
        UndoableModelStatePair const&
    );

    // try to automatically set the model's scale factor based on how big the scene is
    bool ActionAutoscaleSceneScaleFactor(
        UndoableModelStatePair&
    );

    // toggle model frame visibility
    bool ActionToggleFrames(
        UndoableModelStatePair&
    );

    // toggle model marker visibility
    bool ActionToggleMarkers(
        UndoableModelStatePair&
    );

    // toggle contact geometry visibility
    bool ActionToggleContactGeometry(
        UndoableModelStatePair&
    );

    // toggle model wrap geometry visibility
    bool ActionToggleWrapGeometry(
        UndoableModelStatePair&
    );

    // open the parent directory of the model's backing file (if applicable) in an OS file explorer window
    bool ActionOpenOsimParentDirectory(
        UndoableModelStatePair&
    );

    // open the model's backing file (if applicable) in an OS-determined default for osims
    bool ActionOpenOsimInExternalEditor(
        UndoableModelStatePair&
    );

    // force a reload of the model, and its associated assets, from its backing file
    bool ActionReloadOsimFromDisk(
        UndoableModelStatePair&,
        SceneCache&
    );

    // start performing a series of simulations against the model by opening a tab that tries all possible integrators
    bool ActionSimulateAgainstAllIntegrators(
        ParentPtr<MainUIStateAPI> const&,
        UndoableModelStatePair const&
    );

    // add an offset frame to the current selection (if applicable)
    bool ActionAddOffsetFrameToPhysicalFrame(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& physFramePath
    );

    // returns true if the specified joint (if valid) can be re-zeroed
    bool CanRezeroJoint(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& jointPath
    );

    // re-zeroes the selected joint (if applicable)
    bool ActionRezeroJoint(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& jointPath
    );

    // adds a parent offset frame to the selected joint (if applicable)
    bool ActionAddParentOffsetFrameToJoint(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& jointPath
    );

    // adds a child offset frame to the selected joint (if applicable)
    bool ActionAddChildOffsetFrameToJoint(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& jointPath
    );

    // sets the name of the selected component (if applicable)
    bool ActionSetComponentName(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& componentPath,
        std::string const&
    );

    // changes the type of the selected joint (if applicable) to the provided joint
    bool ActionChangeJointTypeTo(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& jointPath,
        std::unique_ptr<OpenSim::Joint>
    );

    // attaches geometry to the selected physical frame (if applicable)
    bool ActionAttachGeometryToPhysicalFrame(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& physFramePath,
        std::unique_ptr<OpenSim::Geometry>
    );

    // assigns contact geometry to the selected HCF (if applicable)
    bool ActionAssignContactGeometryToHCF(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& hcfPath,
        OpenSim::ComponentPath const& contactGeomPath
    );

    // applies a property edit to the model
    bool ActionApplyPropertyEdit(
        UndoableModelStatePair&,
        ObjectPropertyEdit&
    );

    // adds a path point to the selected path actuator (if applicable)
    bool ActionAddPathPointToPathActuator(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& pathActuatorPath,
        OpenSim::ComponentPath const& pointPhysFrame
    );

    // flags for reassignment
    enum class SocketReassignmentFlags {
        None,
        TryReexpressComponentInNewConnectee,
    };
    constexpr bool operator&(SocketReassignmentFlags a, SocketReassignmentFlags b)
    {
        return osc::to_underlying(a) & osc::to_underlying(b);
    }

    // attempts to reassign a component's socket connection (returns false and writes to `error` on failure)
    bool ActionReassignComponentSocket(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const& componentAbsPath,
        std::string const& socketName,
        OpenSim::Object const& connectee,
        SocketReassignmentFlags flags,
        std::string& error
    );

    // sets the model's scale factor
    bool ActionSetModelSceneScaleFactorTo(
        UndoableModelStatePair&,
        float
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
    bool ActionAddBodyToModel(
        UndoableModelStatePair&,
        BodyDetails const&
    );

    // add the given component into the model graph
    bool ActionAddComponentToModel(
        UndoableModelStatePair&,
        std::unique_ptr<OpenSim::Component>,
        std::string& errorOut
    );

    // set the speed of a coordinate
    bool ActionSetCoordinateSpeed(
        UndoableModelStatePair&,
        OpenSim::Coordinate const&,
        double newSpeed
    );

    // set the speed of a coordinate and ensure it is saved
    bool ActionSetCoordinateSpeedAndSave(
        UndoableModelStatePair&,
        OpenSim::Coordinate const&,
        double newSpeed
    );

    // set a coordinate (un)locked
    bool ActionSetCoordinateLockedAndSave(
        UndoableModelStatePair&,
        OpenSim::Coordinate const&,
        bool
    );

    // set the value of a coordinate
    bool ActionSetCoordinateValue(
        UndoableModelStatePair&,
        OpenSim::Coordinate const&,
        double newValue
    );

    // set the value of a coordinate and ensure it is saved
    bool ActionSetCoordinateValueAndSave(
        UndoableModelStatePair&,
        OpenSim::Coordinate const&,
        double newValue
    );

    // sets the `Appearance` property of the pointed-to component, and all its children, to have visible = bool
    bool ActionSetComponentAndAllChildrensIsVisibleTo(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const&,
        bool newVisibility
    );

    // sets the `Appearance` property of all components in the model to `visible = false`, followed by setting the
    // `Appearance` property of the pointed-to component, and all its children, to `visible = true`
    bool ActionShowOnlyComponentAndAllChildren(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const&
    );

    // sets the `Appearance` property of all components in the model to `visible = visible` if that component has
    // the given concrete class name
    bool ActionSetComponentAndAllChildrenWithGivenConcreteClassNameIsVisibleTo(
        UndoableModelStatePair&,
        OpenSim::ComponentPath const&,
        std::string_view concreteClassName,
        bool newVisibility
    );

    // sets the location of the given station in its parent frame to its old location plus the provided delta
    //
    // (does not save the change to undo/redo storage)
    bool ActionTranslateStation(
        UndoableModelStatePair&,
        OpenSim::Station const&,
        Vec3 const& deltaPosition
    );

    // sets the location of the given station in its parent frame to its old location plus the provided vector
    //
    // (saves the change to undo/redo storage)
    bool ActionTranslateStationAndSave(
        UndoableModelStatePair&,
        OpenSim::Station const&,
        Vec3 const& deltaPosition
    );

    // sets the location of the given path point in its parent frame to its old location plus the provided delta
    //
    // (does not save the change to undo/redo storage)
    bool ActionTranslatePathPoint(
        UndoableModelStatePair&,
        OpenSim::PathPoint const&,
        Vec3 const& deltaPosition
    );

    // sets the location of the given path point in its parent frame to its old location plus the provided delta
    //
    // (saves the change to undo/redo storage)
    bool ActionTranslatePathPointAndSave(
        UndoableModelStatePair&,
        OpenSim::PathPoint const&,
        Vec3 const& deltaPosition
    );

    bool ActionTransformPof(
        UndoableModelStatePair&,
        OpenSim::PhysicalOffsetFrame const&,
        Vec3 const& deltaTranslationInParentFrame,
        Vec3 const& newPofEulers
    );

    bool ActionTransformWrapObject(
        UndoableModelStatePair&,
        OpenSim::WrapObject const&,
        Vec3 const& deltaPosition,
        Vec3 const& newEulers
    );

    bool ActionTransformContactGeometry(
        UndoableModelStatePair&,
        OpenSim::ContactGeometry const&,
        Vec3 const& deltaPosition,
        Vec3 const& newEulers
    );

    bool ActionFitSphereToMesh(
        UndoableModelStatePair&,
        OpenSim::Mesh const&
    );
    bool ActionFitEllipsoidToMesh(
        UndoableModelStatePair&,
        OpenSim::Mesh const&
    );
    bool ActionFitPlaneToMesh(
        UndoableModelStatePair&,
        OpenSim::Mesh const&
    );
}

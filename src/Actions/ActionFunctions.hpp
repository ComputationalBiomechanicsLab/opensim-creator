#pragma once

#include <glm/vec3.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace OpenSim { class ContactGeometry; }
namespace OpenSim { class Coordinate; }
namespace OpenSim { class Geometry; }
namespace OpenSim { class Joint; }
namespace OpenSim { class Object; }
namespace OpenSim { class PhysicalFrame; }
namespace osc { class MainUIStateAPI; }
namespace osc { class ObjectPropertyEdit; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    // prompt the user for a save location and then save the model to the specified location
    void ActionSaveCurrentModelAs(osc::UndoableModelStatePair&);

    // create a new model and show it in a new tab
    void ActionNewModel(MainUIStateAPI*);

    // prompt a user to open a model file and open it in a new tab
    void ActionOpenModel(MainUIStateAPI*);

    // open the specified model in a loading tab
    void ActionOpenModel(MainUIStateAPI*, std::filesystem::path const&);

    // try to save the given model file to disk
    bool ActionSaveModel(MainUIStateAPI*, UndoableModelStatePair&);

    // try to delete an undoable-model's current selection
    //
    // "try", because some things are difficult to delete from OpenSim models
    void ActionTryDeleteSelectionFromEditedModel(UndoableModelStatePair&);

    // try to undo currently edited model to earlier state
    void ActionUndoCurrentlyEditedModel(UndoableModelStatePair&);

    // try to redo currently edited model to later state
    void ActionRedoCurrentlyEditedModel(UndoableModelStatePair&);

    // disable all wrapping surfaces in the current model
    void ActionDisableAllWrappingSurfaces(UndoableModelStatePair&);

    // enable all wrapping surfaces in the current model
    void ActionEnableAllWrappingSurfaces(UndoableModelStatePair&);

    // clears the current selection in the model
    void ActionClearSelectionFromEditedModel(UndoableModelStatePair&);

    // loads an STO file against the current model and opens it in a new tab
    bool ActionLoadSTOFileAgainstModel(MainUIStateAPI*, UndoableModelStatePair const&, std::filesystem::path stoPath);

    // start simulating the given model in a forward-dynamic simulator tab
    bool ActionStartSimulatingModel(MainUIStateAPI*, UndoableModelStatePair const&);

    // reload the given model from its backing file (if applicable)
    bool ActionUpdateModelFromBackingFile(UndoableModelStatePair&);

    // copies the full absolute path to the osim to the clipboard
    bool ActionCopyModelPathToClipboard(UndoableModelStatePair const&);

    // try to automatically set the model's scale factor based on how big the scene is
    bool ActionAutoscaleSceneScaleFactor(UndoableModelStatePair&);

    // toggle model frame visibility
    bool ActionToggleFrames(UndoableModelStatePair&);

    // open the parent directory of the model's backing file (if applicable) in an OS file explorer window
    bool ActionOpenOsimParentDirectory(UndoableModelStatePair&);

    // open the model's backing file (if applicable) in an OS-determined default for osims
    bool ActionOpenOsimInExternalEditor(UndoableModelStatePair&);

    // force a reload of the model from its backing file (if applicable)
    bool ActionReloadOsimFromDisk(UndoableModelStatePair&);

    // start performing a series of simulations against the model by opening a tab that tries all possible integrators
    bool ActionSimulateAgainstAllIntegrators(MainUIStateAPI*, UndoableModelStatePair const&);

    // add an offset frame to the current selection (if applicable)
    bool ActionAddOffsetFrameToPhysicalFrame(UndoableModelStatePair&, OpenSim::ComponentPath const& physFramePath);

    // returns true if the specified joint (if valid) can be re-zeroed
    bool CanRezeroJoint(UndoableModelStatePair&, OpenSim::ComponentPath const& jointPath);

    // re-zeroes the selected joint (if applicable)
    bool ActionRezeroJoint(UndoableModelStatePair&, OpenSim::ComponentPath const& jointPath);

    // adds a parent offset frame to the selected joint (if applicable)
    bool ActionAddParentOffsetFrameToJoint(UndoableModelStatePair&, OpenSim::ComponentPath const& jointPath);

    // adds a child offset frame to the selected joint (if applicable)
    bool ActionAddChildOffsetFrameToJoint(UndoableModelStatePair&, OpenSim::ComponentPath const& jointPath);

    // sets the name of the selected component (if applicable)
    bool ActionSetComponentName(UndoableModelStatePair&, OpenSim::ComponentPath const& componentPath, std::string const&);

    // changes the type of the selected joint (if applicable) to the provided joint
    bool ActionChangeJointTypeTo(UndoableModelStatePair&, OpenSim::ComponentPath const& jointPath, std::unique_ptr<OpenSim::Joint>);

    // attaches geometry to the selected physical frame (if applicable)
    bool ActionAttachGeometryToPhysicalFrame(UndoableModelStatePair&, OpenSim::ComponentPath const& physFramePath, std::unique_ptr<OpenSim::Geometry>);

    // assigns contact geometry to the selected HCF (if applicable)
    bool ActionAssignContactGeometryToHCF(UndoableModelStatePair&, OpenSim::ComponentPath const& hcfPath, OpenSim::ComponentPath const& contactGeomPath);

    // applies a property edit to the model
    bool ActionApplyPropertyEdit(UndoableModelStatePair&, ObjectPropertyEdit&);

    // adds a path point to the selected path actuator (if applicable)
    bool ActionAddPathPointToPathActuator(UndoableModelStatePair&, OpenSim::ComponentPath const& pathActuatorPath, OpenSim::ComponentPath const& pointPhysFrame);

    // attempts to reassign a component's socket connection (returns false and writes to `error` on failure)
    bool ActionReassignComponentSocket(UndoableModelStatePair&, OpenSim::ComponentPath const& componentAbsPath, std::string const& socketName, OpenSim::Object const& connectee, std::string& error);

    // sets the model's showing only to the provided component (can be nullptr)
    bool ActionSetModelShowingOnlyTo(UndoableModelStatePair&, OpenSim::Component const*);

    // sets the model's scale factor
    bool ActionSetModelSceneScaleFactorTo(UndoableModelStatePair&, float);

    // details of a body that should be added too a model
    struct BodyDetails {
        glm::vec3 CenterOfMass;
        glm::vec3 Inertia;
        float Mass;
        std::string ParentFrameAbsPath;
        std::string BodyName;
        int JointTypeIndex;
        std::string JointName;
        std::unique_ptr<OpenSim::Geometry> MaybeGeometry;
        bool AddOffsetFrames;

        BodyDetails();
    };

    // add a new body to the model
    bool ActionAddBodyToModel(UndoableModelStatePair&, BodyDetails const&);

    // add the given component into the model graph
    bool ActionAddComponentToModel(UndoableModelStatePair&, std::unique_ptr<OpenSim::Component>);

    // set the speed of a coordinate
    bool ActionSetCoordinateSpeed(UndoableModelStatePair&, OpenSim::Coordinate const&, double);

    // set the speed of a coordinate and ensure it is saved
    bool ActionSetCoordinateSpeedAndSave(UndoableModelStatePair&, OpenSim::Coordinate const&, double);

    // set a coordinate (un)locked
    bool ActionSetCoordinateLockedAndSave(UndoableModelStatePair&, OpenSim::Coordinate const&, bool);

    // set the value of a coordinate
    bool ActionSetCoordinateValue(UndoableModelStatePair&, OpenSim::Coordinate const&, double);

    // set the value of a coordinate and ensure it is saved
    bool ActionSetCoordinateValueAndSave(UndoableModelStatePair&, OpenSim::Coordinate const&, double);
}

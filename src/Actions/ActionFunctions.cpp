#include "ActionFunctions.hpp"

#include "src/Bindings/SimTKHelpers.hpp"
#include "src/MiddlewareAPIs/MainUIStateAPI.hpp"
#include "src/OpenSimBindings/BasicModelStatePair.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulation.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulatorParams.hpp"
#include "src/OpenSimBindings/ForwardDynamicSimulator.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/Simulation.hpp"
#include "src/OpenSimBindings/StoFileSimulation.hpp"
#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Tabs/ModelEditorTab.hpp"
#include "src/Tabs/LoadingTab.hpp"
#include "src/Tabs/SimulatorTab.hpp"
#include "src/Tabs/PerformanceAnalyzerTab.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/UID.hpp"
#include "src/Widgets/ObjectPropertiesEditor.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

#include <memory>
#include <stdexcept>
#include <utility>


static void OpenOsimInLoadingTab(osc::MainUIStateAPI* api, std::filesystem::path p)
{
    osc::UID tabID = api->addTab<osc::LoadingTab>(api, p);
    api->selectTab(tabID);
}

static void DoOpenFileViaDialog(osc::MainUIStateAPI* api)
{
    std::filesystem::path p = osc::PromptUserForFile("osim");
    if (!p.empty())
    {
        OpenOsimInLoadingTab(api, p);
    }
}

static std::optional<std::filesystem::path> PromptSaveOneFile()
{
    std::filesystem::path p = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("osim");
    return !p.empty() ? std::optional{p} : std::nullopt;
}

static bool IsAnExampleFile(std::filesystem::path const& path)
{
    return osc::IsSubpath(osc::App::resource("models"), path);
}

static std::optional<std::string> TryGetModelSaveLocation(OpenSim::Model const& m)
{
    if (std::string const& backing_path = m.getInputFileName();
        backing_path != "Unassigned" && backing_path.size() > 0)
    {
        // the model has an associated file
        //
        // we can save over this document - *IF* it's not an example file
        if (IsAnExampleFile(backing_path))
        {
            auto maybePath = PromptSaveOneFile();
            return maybePath ? std::optional<std::string>{maybePath->string()} : std::nullopt;
        }
        else
        {
            return backing_path;
        }
    }
    else
    {
        // the model has no associated file, so prompt the user for a save
        // location
        auto maybePath = PromptSaveOneFile();
        return maybePath ? std::optional<std::string>{maybePath->string()} : std::nullopt;
    }
}

static bool TrySaveModel(OpenSim::Model const& model, std::string const& save_loc)
{
    try
    {
        model.print(save_loc);
        osc::log::info("saved model to %s", save_loc.c_str());
        return true;
    }
    catch (OpenSim::Exception const& ex)
    {
        osc::log::error("error saving model: %s", ex.what());
        return false;
    }
}

void osc::ActionSaveCurrentModelAs(osc::UndoableModelStatePair& uim)
{
    auto maybePath = PromptSaveOneFile();

    if (maybePath && TrySaveModel(uim.getModel(), maybePath->string()))
    {
        std::string oldPath = uim.getModel().getInputFileName();

        uim.updModel().setInputFileName(maybePath->string());
        uim.setFilesystemPath(*maybePath);
        uim.setUpToDateWithFilesystem();

        if (*maybePath != oldPath)
        {
            uim.commit("set model path");
        }

        osc::App::upd().addRecentFile(*maybePath);
    }
}

void osc::ActionNewModel(MainUIStateAPI* api)
{

    auto p = std::make_unique<UndoableModelStatePair>();
    UID tabID = api->addTab<ModelEditorTab>(api, std::move(p));
    api->selectTab(tabID);
}

void osc::ActionOpenModel(MainUIStateAPI* api)
{
    DoOpenFileViaDialog(api);
}

void osc::ActionOpenModel(MainUIStateAPI* api, std::filesystem::path const& path)
{
    OpenOsimInLoadingTab(api, path);
}

bool osc::ActionSaveModel(MainUIStateAPI* api, UndoableModelStatePair& model)
{
    std::optional<std::string> maybeUserSaveLoc = TryGetModelSaveLocation(model.getModel());

    if (maybeUserSaveLoc && TrySaveModel(model.getModel(), *maybeUserSaveLoc))
    {
        std::string oldPath = model.getModel().getInputFileName();
        model.updModel().setInputFileName(*maybeUserSaveLoc);
        model.setFilesystemPath(*maybeUserSaveLoc);
        model.setUpToDateWithFilesystem();

        if (*maybeUserSaveLoc != oldPath)
        {
            model.commit("set model path");
        }

        osc::App::upd().addRecentFile(*maybeUserSaveLoc);
        return true;
    }
    else
    {
        return false;
    }
}

void osc::ActionTryDeleteSelectionFromEditedModel(osc::UndoableModelStatePair& uim)
{
    OpenSim::Component const* selected = uim.getSelected();

    if (!selected)
    {
        return;
    }

    OpenSim::ComponentPath selectedPath = selected->getAbsolutePath();

    UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::Component* mutComponent = osc::FindComponentMut(mutModel, selectedPath);

    if (!mutComponent)
    {
        uim.setModelVersion(oldVersion);
    }
    else if (osc::TryDeleteComponentFromModel(mutModel, *mutComponent))
    {
        osc::InitializeModel(mutModel);
        osc::InitializeState(mutModel);
        uim.commit("deleted compopnent");
    }
    else
    {
        uim.setModelVersion(oldVersion);
    }
}

void osc::ActionUndoCurrentlyEditedModel(osc::UndoableModelStatePair& model)
{
    if (model.canUndo())
    {
        model.doUndo();
    }
}

void osc::ActionRedoCurrentlyEditedModel(osc::UndoableModelStatePair& model)
{
    if (model.canRedo())
    {
        model.doRedo();
    }
}

// disable all wrapping surfaces in the current model
void osc::ActionDisableAllWrappingSurfaces(osc::UndoableModelStatePair& model)
{
    OpenSim::Model& mutModel = model.updModel();
    osc::DeactivateAllWrapObjectsIn(mutModel);
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    model.commit("disabled all wrapping surfaces");
}

// enable all wrapping surfaces in the current model
void osc::ActionEnableAllWrappingSurfaces(osc::UndoableModelStatePair& model)
{
    OpenSim::Model& mutModel = model.updModel();
    osc::ActivateAllWrapObjectsIn(mutModel);
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    model.commit("enabled all wrapping surfaces");
}

void osc::ActionClearSelectionFromEditedModel(osc::UndoableModelStatePair& model)
{
    model.setSelected(nullptr);
}

bool osc::ActionLoadSTOFileAgainstModel(osc::MainUIStateAPI* parent, osc::UndoableModelStatePair const& uim, std::filesystem::path stoPath)
{
    try
    {
        std::unique_ptr<OpenSim::Model> cpy = std::make_unique<OpenSim::Model>(uim.getModel());
        osc::InitializeModel(*cpy);
        osc::InitializeState(*cpy);

        osc::UID tabID = parent->addTab<osc::SimulatorTab>(parent, std::make_shared<osc::Simulation>(osc::StoFileSimulation{std::move(cpy), stoPath, uim.getFixupScaleFactor()}));
        parent->selectTab(tabID);

        return true;
    }
    catch (std::exception const& ex)
    {
        osc::log::error("encountered error while trying to load an STO file against the model: %s", ex.what());
    }
    return false;
}

bool osc::ActionStartSimulatingModel(osc::MainUIStateAPI* parent, osc::UndoableModelStatePair const& uim)
{
    osc::BasicModelStatePair modelState{uim};
    osc::ForwardDynamicSimulatorParams params = osc::FromParamBlock(parent->getSimulationParams());

    auto sim = std::make_shared<osc::Simulation>(osc::ForwardDynamicSimulation{std::move(modelState), std::move(params)});
    auto tab = std::make_unique<osc::SimulatorTab>(parent, std::move(sim));

    parent->selectTab(parent->addTab(std::move(tab)));

    return true;
}

bool osc::ActionUpdateModelFromBackingFile(osc::UndoableModelStatePair& uim)
{
    try
    {
        osc::log::info("file change detected: loading updated file");
        auto p = std::make_unique<OpenSim::Model>(uim.getModel().getInputFileName());
        osc::log::info("loaded updated file");
        uim.setModel(std::move(p));
        uim.setUpToDateWithFilesystem();
        uim.commit("reloaded model from filesystem");
        return true;
    }
    catch (std::exception const& ex)
    {
        osc::log::error("error occurred while trying to automatically load a model file:");
        osc::log::error(ex.what());
        osc::log::error("the file will not be loaded into osc (you won't see the change in the UI)");
        return false;
    }
}

bool osc::ActionAutoscaleSceneScaleFactor(osc::UndoableModelStatePair& uim)
{
    float sf = osc::GetRecommendedScaleFactor(uim);
    uim.setFixupScaleFactor(sf);
    return true;
}

bool osc::ActionToggleFrames(osc::UndoableModelStatePair& uim)
{
    OpenSim::Model& mutModel = uim.updModel();
    bool showingFrames = mutModel.get_ModelVisualPreferences().get_ModelDisplayHints().get_show_frames();
    mutModel.upd_ModelVisualPreferences().upd_ModelDisplayHints().set_show_frames(!showingFrames);
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.commit("edited frame visibility");
    return true;
}

bool osc::ActionOpenOsimParentDirectory(osc::UndoableModelStatePair& uim)
{
    bool hasBackingFile = osc::HasInputFileName(uim.getModel());

    if (hasBackingFile)
    {
        std::filesystem::path p{uim.getModel().getInputFileName()};
        osc::OpenPathInOSDefaultApplication(p.parent_path());
        return true;
    }
    else
    {
        return false;
    }
}

bool osc::ActionOpenOsimInExternalEditor(osc::UndoableModelStatePair& uim)
{
    bool hasBackingFile = osc::HasInputFileName(uim.getModel());

    if (hasBackingFile)
    {
        osc::OpenPathInOSDefaultApplication(uim.getModel().getInputFileName());
        return true;
    }
    else
    {
        return false;
    }
}

bool osc::ActionReloadOsimFromDisk(osc::UndoableModelStatePair& uim)
{
    bool hasBackingFile = osc::HasInputFileName(uim.getModel());

    if (hasBackingFile)
    {
        try
        {
            osc::log::info("manual osim file reload requested: attempting to reload the file");
            auto p = std::make_unique<OpenSim::Model>(uim.getModel().getInputFileName());
            osc::log::info("loaded updated file");
            uim.setModel(std::move(p));
            uim.setUpToDateWithFilesystem();
            uim.commit("reloaded model from filesystem");
            return true;
        }
        catch (std::exception const& ex)
        {
            osc::log::error("error occurred while trying to reload a model file:");
            osc::log::error(ex.what());
            return false;
        }
    }
    else
    {
        osc::log::error("cannot reload the osim file: the model doesn't appear to have a backing file (is it saved?)");
        return false;
    }
}

bool osc::ActionSimulateAgainstAllIntegrators(osc::MainUIStateAPI* parent, osc::UndoableModelStatePair const& uim)
{
    osc::UID tabID = parent->addTab<osc::PerformanceAnalyzerTab>(parent, osc::BasicModelStatePair{uim}, parent->getSimulationParams());
    parent->selectTab(tabID);
    return true;
}

bool osc::ActionAddOffsetFrameToSelection(osc::UndoableModelStatePair& uim)
{
    OpenSim::PhysicalFrame const* selection = uim.getSelectedAs<OpenSim::PhysicalFrame>();

    if (!selection)
    {
        return false;
    }

    OpenSim::ComponentPath selectionPath = selection->getAbsolutePath();

    auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pof->setName(selection->getName() + "_offsetframe");
    pof->setParentFrame(*selection);

    auto pofptr = pof.get();

    UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::PhysicalFrame* mutSelection = osc::FindComponentMut<OpenSim::PhysicalFrame>(mutModel, selectionPath);

    if (!mutSelection)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    mutSelection->addComponent(pof.release());
    mutModel.finalizeConnections();
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.setSelected(pofptr);
    uim.commit("added offset frame");

    return true;
}

bool osc::CanRezeroSelectedJoint(osc::UndoableModelStatePair& uim)
{
    OpenSim::Joint const* selection = uim.getSelectedAs<OpenSim::Joint>();

    if (!selection)
    {
        return false;
    }

    // if the joint uses offset frames for both its parent and child frames then
    // it is possible to reorient those frames such that the joint's new zero
    // point is whatever the current arrangement is (effectively, by pre-transforming
    // the parent into the child and assuming a "zeroed" joint is an identity op)

    return osc::DerivesFrom<OpenSim::PhysicalOffsetFrame>(selection->getParentFrame());
}

bool osc::ActionRezeroSelectedJoint(osc::UndoableModelStatePair& uim)
{
    OpenSim::Joint const* selection = uim.getSelectedAs<OpenSim::Joint>();

    if (!selection)
    {
        return false;
    }

    OpenSim::PhysicalOffsetFrame const* parentPOF = dynamic_cast<OpenSim::PhysicalOffsetFrame const*>(&selection->getParentFrame());

    if (!parentPOF)
    {
        return false;
    }

    OpenSim::PhysicalFrame const& childFrame = selection->getChildFrame();

    SimTK::Transform parentXform = parentPOF->getTransformInGround(uim.getState());
    SimTK::Transform childXform = childFrame.getTransformInGround(uim.getState());
    SimTK::Transform child2parent = parentXform.invert() * childXform;
    SimTK::Transform newXform = parentPOF->getOffsetTransform() * child2parent;

    OpenSim::ComponentPath parentPath = parentPOF->getAbsolutePath();

    UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::PhysicalOffsetFrame* mutParent = osc::FindComponentMut<OpenSim::PhysicalOffsetFrame>(mutModel, parentPath);

    if (!mutParent)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    mutParent->setOffsetTransform(newXform);
    mutModel.finalizeConnections();
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.commit("rezeroed joint");

    return true;
}

bool osc::ActionAddParentOffsetFrameToSelectedJoint(osc::UndoableModelStatePair& uim)
{
    OpenSim::Joint const* selection = uim.getSelectedAs<OpenSim::Joint>();

    if (!selection)
    {
        return false;
    }

    OpenSim::ComponentPath selectionPath = selection->getAbsolutePath();

    auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pf->setParentFrame(selection->getParentFrame());

    osc::UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::Joint* mutJoint = osc::FindComponentMut<OpenSim::Joint>(mutModel, selectionPath);

    if (!mutJoint)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    mutJoint->addFrame(pf.release());
    mutModel.finalizeConnections();
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.commit("added parent offset frame");

    return true;
}

bool osc::ActionAddChildOffsetFrameToSelectedJoint(osc::UndoableModelStatePair& uim)
{
    OpenSim::Joint const* selection = uim.getSelectedAs<OpenSim::Joint>();

    if (!selection)
    {
        return false;
    }

    OpenSim::ComponentPath selectionPath = selection->getAbsolutePath();

    auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pf->setParentFrame(selection->getChildFrame());

    UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::Joint* mutJoint = osc::FindComponentMut<OpenSim::Joint>(mutModel, selectionPath);

    if (!mutJoint)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    mutJoint->addFrame(pf.release());
    mutModel.finalizeConnections();
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.commit("added child offset frame");

    return true;
}

bool osc::ActionSetSelectedComponentName(osc::UndoableModelStatePair& uim, std::string const& newName)
{
    if (newName.empty())
    {
        return false;
    }

    OpenSim::Component const* selection = uim.getSelected();

    if (!selection)
    {
        return false;
    }

    OpenSim::ComponentPath selectionPath = selection->getAbsolutePath();

    UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::Component* mutComponent = osc::FindComponentMut(mutModel, selectionPath);

    if (!mutComponent)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    mutComponent->setName(newName);
    mutModel.finalizeConnections();  // because pointers need to know the new name
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.setSelected(mutComponent);  // because the name changed
    uim.commit("changed component name");

    return true;
}

bool osc::ActionChangeSelectedJointTypeTo(osc::UndoableModelStatePair& uim, std::unique_ptr<OpenSim::Joint> newType)
{
    OpenSim::Joint const* selection = uim.getSelectedAs<OpenSim::Joint>();

    if (!selection)
    {
        return false;
    }

    OpenSim::JointSet const* owner = osc::GetOwner<OpenSim::JointSet>(*selection);

    if (!owner)
    {
        return false;
    }

    OpenSim::ComponentPath ownerPath = owner->getAbsolutePath();

    int idx = FindJointInParentJointSet(*selection);

    if (idx == -1)
    {
        return false;
    }

    osc::CopyCommonJointProperties(*selection, *newType);

    // update: overwrite old joint in model
    //
    // note: this will invalidate the `selection` joint, because the
    // OpenSim::JointSet container will automatically kill it

    UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::JointSet* mutParent = osc::FindComponentMut<OpenSim::JointSet>(mutModel, ownerPath);
    OpenSim::Joint* ptr = newType.get();

    mutParent->set(idx, newType.release());
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.setSelected(ptr);
    uim.commit("changed joint type");

    return true;
}

bool osc::ActionAttachGeometryToSelectedPhysicalFrame(osc::UndoableModelStatePair& uim, std::unique_ptr<OpenSim::Geometry> geom)
{
    OpenSim::PhysicalFrame const* pof = uim.getSelectedAs<OpenSim::PhysicalFrame>();

    if (!pof)
    {
        return false;
    }

    OpenSim::ComponentPath pofPath = pof->getAbsolutePath();

    UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::PhysicalFrame* mutPof = osc::FindComponentMut<OpenSim::PhysicalFrame>(mutModel, pofPath);

    if (!mutPof)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    mutPof->attachGeometry(geom.release());
    mutModel.finalizeConnections();
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.commit("attached geometry");

    return true;
}

bool osc::ActionAssignContactGeometryToSelectedHCF(osc::UndoableModelStatePair& uim, OpenSim::ContactGeometry const& geom)
{
    OpenSim::HuntCrossleyForce const* hcf = uim.getSelectedAs<OpenSim::HuntCrossleyForce>();

    if (!hcf)
    {
        return false;
    }

    OpenSim::ComponentPath selectionPath = hcf->getAbsolutePath();

    UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::HuntCrossleyForce* mutHCF = osc::FindComponentMut<OpenSim::HuntCrossleyForce>(mutModel, selectionPath);

    if (!mutHCF)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    // TODO: this should be done when the model is initially loaded

    // HACK: if it has no parameters, give it some. The HuntCrossleyForce implementation effectively
    // does this internally anyway to satisfy its own API (e.g. `getStaticFriction` requires that
    // the HuntCrossleyForce has a parameter)
    if (mutHCF->get_contact_parameters().getSize() == 0)
    {
        mutHCF->updContactParametersSet().adoptAndAppend(new OpenSim::HuntCrossleyForce::ContactParameters());
    }

    mutHCF->updContactParametersSet()[0].updGeometry().appendValue(geom.getName());
    mutModel.finalizeConnections();
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.commit("added contact geometry");

    return true;
}

bool osc::ActionApplyPropertyEdit(osc::UndoableModelStatePair& uim, ObjectPropertyEdit& resp)
{
    UID oldVersion = uim.getModelVersion();
    OpenSim::Model& model = uim.updModel();

    OpenSim::Component* component = osc::FindComponentMut(model, resp.getComponentAbsPath());

    if (!component)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    OpenSim::AbstractProperty* property = osc::FindPropertyMut(*component, resp.getPropertyName());

    if (!property)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    resp.apply(*property);
    osc::InitializeModel(model);
    osc::InitializeState(model);
    uim.commit("edited property");

    return true;
}

bool osc::ActionAddPathPointToSelectedPathActuator(osc::UndoableModelStatePair& uim, OpenSim::PhysicalFrame const& pf)
{
    OpenSim::PathActuator const* pa = uim.getSelectedAs<OpenSim::PathActuator>();

    if (!pa)
    {
        return false;
    }

    OpenSim::ComponentPath selectionPath = pa->getAbsolutePath();

    int n = pa->getGeometryPath().getPathPointSet().getSize();
    std::string name = pa->getName() + "-P" + std::to_string(n + 1);
    SimTK::Vec3 pos{0.0f, 0.0f, 0.0f};

    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::PathActuator* mutPA = osc::FindComponentMut<OpenSim::PathActuator>(mutModel, selectionPath);
    mutPA->addNewPathPoint(name, pf, pos);
    mutModel.finalizeConnections();
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.commit("added path point to path actuator");
    return true;
}

bool osc::ActionReassignSelectedComponentSocket(osc::UndoableModelStatePair& uim, std::string const& socketName, OpenSim::Object const& connectee, std::string& error)
{
    OpenSim::Component const* selected = uim.getSelected();

    if (!selected)
    {
        return false;
    }

    OpenSim::ComponentPath selectedPath = selected->getAbsolutePath();

    UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::Component* mutComponent = osc::FindComponentMut(mutModel, selectedPath);

    OpenSim::AbstractSocket* socket = nullptr;
    try
    {
        socket = &mutComponent->updSocket(socketName);
    }
    catch (std::exception const&)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    if (!socket)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    OpenSim::Object const& existing = socket->getConnecteeAsObject();

    try
    {
        socket->connect(connectee);
        mutModel.finalizeConnections();
        osc::InitializeModel(mutModel);
        osc::InitializeState(mutModel);
        uim.commit("reassigned socket");
        return true;
    }
    catch (std::exception const& ex)
    {
        error = ex.what();
        socket->connect(existing);
        uim.setModelVersion(oldVersion);
        return false;
    }
}

bool osc::ActionSetModelIsolationTo(osc::UndoableModelStatePair& uim, OpenSim::Component const* c)
{
    uim.setIsolated(c);
    uim.commit("changed isolation");
    return true;
}

bool osc::ActionSetModelSceneScaleFactorTo(osc::UndoableModelStatePair& uim, float v)
{
    uim.setFixupScaleFactor(v);
    return true;
}

osc::BodyDetails::BodyDetails() :
    CenterOfMass{0.0f, 0.0f, 0.0f},
    Inertia{1.0f, 1.0f, 1.0f},
    Mass{1.0f},
    ParentFrameAbsPath{},
    BodyName{"new_body"},
    JointTypeIndex{static_cast<int>(osc::JointRegistry::indexOf<OpenSim::FreeJoint>().value_or(0))},
    JointName{},
    MaybeGeometry{nullptr},
    AddOffsetFrames{true}
{
}

// create a "standard" OpenSim::Joint
static std::unique_ptr<OpenSim::Joint> MakeJoint(
    osc::BodyDetails const& details,
    OpenSim::Body const& b,
    OpenSim::Joint const& jointPrototype,
    OpenSim::PhysicalFrame const& selectedPf)
{
    std::unique_ptr<OpenSim::Joint> copy{jointPrototype.clone()};
    copy->setName(details.JointName);

    if (!details.AddOffsetFrames)
    {
        copy->connectSocket_parent_frame(selectedPf);
        copy->connectSocket_child_frame(b);
    }
    else
    {
        // add first offset frame as joint's parent
        {
            auto pof1 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pof1->setParentFrame(selectedPf);
            pof1->setName(selectedPf.getName() + "_offset");
            copy->addFrame(pof1.get());
            copy->connectSocket_parent_frame(*pof1.release());
        }

        // add second offset frame as joint's child
        {
            auto pof2 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pof2->setParentFrame(b);
            pof2->setName(b.getName() + "_offset");
            copy->addFrame(pof2.get());
            copy->connectSocket_child_frame(*pof2.release());
        }
    }

    return copy;
}

bool osc::ActionAddBodyToModel(UndoableModelStatePair& uim, BodyDetails const& details)
{
    OpenSim::PhysicalFrame const* parent = osc::FindComponent<OpenSim::PhysicalFrame>(uim.getModel(), details.ParentFrameAbsPath);

    if (!parent)
    {
        return false;
    }

    SimTK::Vec3 com = ToSimTKVec3(details.CenterOfMass);
    SimTK::Inertia inertia = ToSimTKInertia(details.Inertia);
    double mass = static_cast<double>(details.Mass);

    // create body
    auto body = std::make_unique<OpenSim::Body>(details.BodyName, mass, com, inertia);

    // create joint between body and whatever the frame is
    OpenSim::Joint const& jointProto = *osc::JointRegistry::prototypes()[static_cast<size_t>(details.JointTypeIndex)];
    auto joint = MakeJoint(details, *body, jointProto, *parent);

    // attach decorative geom
    if (details.MaybeGeometry)
    {
        body->attachGeometry(details.MaybeGeometry->clone());
    }

    // mutate the model and perform the edit
    OpenSim::Model& mutModel = uim.updModel();
    mutModel.addJoint(joint.release());
    OpenSim::Body* ptr = body.get();
    mutModel.addBody(body.release());
    mutModel.finalizeConnections();
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    uim.setSelected(ptr);
    uim.commit("added body");

    return true;
}

bool osc::ActionAddComponentToModel(UndoableModelStatePair& model, std::unique_ptr<OpenSim::Component> c)
{
    OpenSim::Model& mutModel = model.updModel();
    auto* ptr = c.get();
    AddComponentToModel(mutModel, std::move(c));
    mutModel.finalizeConnections();
    osc::InitializeModel(mutModel);
    osc::InitializeState(mutModel);
    model.setSelected(ptr);
    model.commit("added component");
    return true;
}

bool osc::ActionSetCoordinateSpeed(UndoableModelStatePair& model, OpenSim::Coordinate const& coord, double v)
{
    auto coordPath = coord.getAbsolutePath();

    UID oldVersion = model.getModelVersion();
    OpenSim::Model& mutModel = model.updModel();
    OpenSim::Coordinate* mutCoord = osc::FindComponentMut<OpenSim::Coordinate>(mutModel, coordPath);

    if (!mutCoord)
    {
        // can't find the coordinate within the provided model
        model.setModelVersion(oldVersion);
        return false;
    }

    // PERF HACK: don't do a full model+state re-realization here: only do it
    //            when the caller wants to save the coordinate change
    mutCoord->setDefaultSpeedValue(v);
    mutCoord->setSpeedValue(mutModel.updWorkingState(), v);
    mutModel.equilibrateMuscles(mutModel.updWorkingState());
    mutModel.realizeDynamics(mutModel.updWorkingState());

    return true;
}

bool osc::ActionSetCoordinateSpeedAndSave(UndoableModelStatePair& model, OpenSim::Coordinate const& coord, double v)
{
    if (ActionSetCoordinateSpeed(model, coord, v))
    {
        OpenSim::Model& mutModel = model.updModel();
        osc::InitializeModel(mutModel);
        osc::InitializeState(mutModel);
        model.commit("set coordinate speed");
        return true;
    }
    else
    {
        // edit wasn't made
        return false;
    }
}

bool osc::ActionSetCoordinateLockedAndSave(UndoableModelStatePair& model, OpenSim::Coordinate const& coord, bool v)
{

    auto coordPath = coord.getAbsolutePath();

    UID oldVersion = model.getModelVersion();
    OpenSim::Model& mutModel = model.updModel();
    OpenSim::Coordinate* mutCoord = osc::FindComponentMut<OpenSim::Coordinate>(mutModel, coordPath);

    if (!mutCoord)
    {
        // can't find the coordinate within the provided model
        model.setModelVersion(oldVersion);
        return false;
    }

    // PERF HACK: don't do a full model+state re-realization here: only do it
    //            when the caller wants to save the coordinate change
    mutCoord->setDefaultLocked(v);
    mutCoord->setLocked(mutModel.updWorkingState(), v);
    mutModel.equilibrateMuscles(mutModel.updWorkingState());
    mutModel.realizeDynamics(mutModel.updWorkingState());

    return true;
}

// set the value of a coordinate, but don't save it to the model (yet)
bool osc::ActionSetCoordinateValue(UndoableModelStatePair& model, OpenSim::Coordinate const& coord, double v)
{
    auto coordPath = coord.getAbsolutePath();

    UID oldVersion = model.getModelVersion();
    OpenSim::Model& mutModel = model.updModel();
    OpenSim::Coordinate* mutCoord = osc::FindComponentMut<OpenSim::Coordinate>(mutModel, coordPath);

    if (!mutCoord)
    {
        // can't find the coordinate within the provided model
        model.setModelVersion(oldVersion);
        return false;
    }

    double rangeMin = std::min(mutCoord->getRangeMin(), mutCoord->getRangeMax());
    double rangeMax = std::max(mutCoord->getRangeMin(), mutCoord->getRangeMax());

    if (!(rangeMin <= v && v <= rangeMax))
    {
        // the requested edit is outside the coordinate's allowed range
        model.setModelVersion(oldVersion);
        return false;
    }

    // PERF HACK: don't do a full model+state re-realization here: only do it
    //            when the caller wants to save the coordinate change
    mutCoord->setDefaultValue(v);
    mutCoord->setValue(mutModel.updWorkingState(), v);
    mutModel.equilibrateMuscles(mutModel.updWorkingState());
    mutModel.realizeDynamics(mutModel.updWorkingState());

    return true;
}

// set the value of a coordinate and ensure it is saved into the model
bool osc::ActionSetCoordinateValueAndSave(UndoableModelStatePair& model, OpenSim::Coordinate const& coord, double v)
{
    if (ActionSetCoordinateValue(model, coord, v))
    {
        OpenSim::Model& mutModel = model.updModel();
        osc::InitializeModel(mutModel);
        osc::InitializeState(mutModel);
        model.commit("set coordinate value");
        return true;
    }
    else
    {
        // edit wasn't made
        return false;
    }
}
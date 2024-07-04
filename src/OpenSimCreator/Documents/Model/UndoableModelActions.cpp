#include "UndoableModelActions.h"

#include <OpenSimCreator/ComponentRegistry/ComponentRegistry.h>
#include <OpenSimCreator/ComponentRegistry/StaticComponentRegistries.h>
#include <OpenSimCreator/Documents/Model/BasicModelStatePair.h>
#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulation.h>
#include <OpenSimCreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
#include <OpenSimCreator/Documents/Simulation/Simulation.h>
#include <OpenSimCreator/Documents/Simulation/StoFileSimulation.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationGenerator.h>
#include <OpenSimCreator/Graphics/OpenSimDecorationOptions.h>
#include <OpenSimCreator/Platform/RecentFiles.h>
#include <OpenSimCreator/UI/IMainUIStateAPI.h>
#include <OpenSimCreator/UI/LoadingTab.h>
#include <OpenSimCreator/UI/PerformanceAnalyzerTab.h>
#include <OpenSimCreator/UI/ModelEditor/ModelEditorTab.h>
#include <OpenSimCreator/UI/Shared/ObjectPropertiesEditor.h>
#include <OpenSimCreator/UI/Simulation/SimulationTab.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>
#include <OpenSimCreator/Utils/ShapeFitters.h>
#include <OpenSimCreator/Utils/SimTKHelpers.h>

#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentList.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Common/ComponentSocket.h>
#include <OpenSim/Common/Exception.h>
#include <OpenSim/Common/ModelDisplayHints.h>
#include <OpenSim/Common/Property.h>
#include <OpenSim/Common/Set.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Geometry.h>
#include <OpenSim/Simulation/Model/GeometryPath.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/JointSet.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/OffsetFrame.h>
#include <OpenSim/Simulation/Model/PathActuator.h>
#include <OpenSim/Simulation/Model/PathPointSet.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/Model/PhysicalOffsetFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <oscar/Graphics/Scene/SceneCache.h>
#include <oscar/Maths/Ellipsoid.h>
#include <oscar/Maths/EllipsoidFunctions.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Plane.h>
#include <oscar/Maths/Quat.h>
#include <oscar/Maths/Sphere.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/FilesystemHelpers.h>
#include <oscar/Utils/ParentPtr.h>
#include <oscar/Utils/UID.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>

using namespace osc;

// helper functions
namespace
{
    void OpenOsimInLoadingTab(const ParentPtr<IMainUIStateAPI>& api, std::filesystem::path p)
    {
        api->add_and_select_tab<LoadingTab>(api, std::move(p));
    }

    void DoOpenFileViaDialog(const ParentPtr<IMainUIStateAPI>& api)
    {
        for (const auto& path : prompt_user_to_select_files({"osim"})) {
            OpenOsimInLoadingTab(api, path);
        }
    }

    std::optional<std::filesystem::path> PromptSaveOneFile()
    {
        return prompt_user_for_file_save_location_add_extension_if_necessary("osim");
    }

    bool IsAnExampleFile(const std::filesystem::path& path)
    {
        return is_subpath(App::resource_filepath("models"), path);
    }

    std::optional<std::string> TryGetModelSaveLocation(const OpenSim::Model& m)
    {
        if (const std::string& backing_path = m.getInputFileName();
            !backing_path.empty() && backing_path != "Unassigned")
        {
            // the model has an associated file
            //
            // we can save over this document - *IF* it's not an example file
            if (IsAnExampleFile(backing_path))
            {
                const std::optional<std::filesystem::path> maybePath = PromptSaveOneFile();
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
            const std::optional<std::filesystem::path> maybePath = PromptSaveOneFile();
            return maybePath ? std::optional<std::string>{maybePath->string()} : std::nullopt;
        }
    }

    bool TrySaveModel(const OpenSim::Model& model, const std::string& save_loc)
    {
        try
        {
            model.print(save_loc);
            log_info("saved model to %s", save_loc.c_str());
            return true;
        }
        catch (const OpenSim::Exception& ex)
        {
            log_error("error saving model: %s", ex.what());
            return false;
        }
    }

    // create a "standard" OpenSim::Joint
    std::unique_ptr<OpenSim::Joint> MakeJoint(
        const BodyDetails& details,
        const OpenSim::Body& b,
        const OpenSim::Joint& jointPrototype,
        const OpenSim::PhysicalFrame& selectedPf)
    {
        std::unique_ptr<OpenSim::Joint> copy = Clone(jointPrototype);
        copy->setName(details.jointName);

        if (!details.addOffsetFrames)
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

                // care: ownership change happens here (#642)
                OpenSim::PhysicalOffsetFrame& ref = AddFrame(*copy, std::move(pof1));
                copy->connectSocket_parent_frame(ref);
            }

            // add second offset frame as joint's child
            {
                auto pof2 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                pof2->setParentFrame(b);
                pof2->setName(b.getName() + "_offset");

                // care: ownership change happens here (#642)
                OpenSim::PhysicalOffsetFrame& ref = AddFrame(*copy, std::move(pof2));
                copy->connectSocket_child_frame(ref);
            }
        }

        return copy;
    }

    bool TryReexpressComponentSpatialPropertiesInNewConnectee(
        OpenSim::Component& component,
        const OpenSim::Object& newConnectee,
        const SimTK::State& state)
    {
        const auto* const newFrame = dynamic_cast<const OpenSim::Frame*>(&newConnectee);
        if (!newFrame)
        {
            return false;  // new connectee isn't a frame
        }

        const auto spatialRep = TryGetSpatialRepresentation(component, state);
        if (!spatialRep)
        {
            return false;  // cannot represent the component spatially
        }

        const SimTK::Transform currentParentToGround = spatialRep->parentToGround;
        const SimTK::Transform groundToNewConnectee = newFrame->getTransformInGround(state).invert();
        const SimTK::Transform currentParentToNewConnectee = groundToNewConnectee * currentParentToGround;

        if (auto* positionalProp = FindSimplePropertyMut<SimTK::Vec3>(component, spatialRep->locationVec3PropertyName))
        {
            const SimTK::Vec3 oldPosition = positionalProp->getValue();
            const SimTK::Vec3 newPosition = currentParentToNewConnectee * oldPosition;

            positionalProp->setValue(newPosition);  // update property with new position
        }

        if (spatialRep->maybeOrientationVec3EulersPropertyName)
        {
            if (auto* orientationalProp = FindSimplePropertyMut<SimTK::Vec3>(component, *spatialRep->maybeOrientationVec3EulersPropertyName))
            {
                const SimTK::Rotation currentRotationInGround = spatialRep->parentToGround.R();
                const SimTK::Rotation groundToNewConnecteeRotation = newFrame->getRotationInGround(state).invert();
                const SimTK::Rotation currentParentRotationToNewConnecteeRotation = groundToNewConnecteeRotation * currentRotationInGround;

                const SimTK::Vec3 oldEulers = orientationalProp->getValue();
                SimTK::Rotation oldRotation;
                oldRotation.setRotationToBodyFixedXYZ(oldEulers);
                const SimTK::Rotation newRotation = currentParentRotationToNewConnecteeRotation * oldRotation;
                const SimTK::Vec3 newEulers = newRotation.convertRotationToBodyFixedXYZ();

                orientationalProp->setValue(newEulers);
            }
        }

        return true;
    }

    // updates `appearance` to that of a fitted geometry
    void UpdAppearanceToFittedGeom(OpenSim::Appearance& appearance)
    {
        appearance.set_color({0.0, 1.0, 0.0});
        appearance.set_opacity(0.3);
    }
}

void osc::ActionSaveCurrentModelAs(UndoableModelStatePair& uim)
{
    const std::optional<std::filesystem::path> maybePath = PromptSaveOneFile();

    if (maybePath && TrySaveModel(uim.getModel(), maybePath->string()))
    {
        const std::string oldPath = uim.getModel().getInputFileName();

        uim.updModel().setInputFileName(maybePath->string());
        uim.setFilesystemPath(*maybePath);

        if (*maybePath != oldPath)
        {
            uim.commit("changed osim path");
        }
        uim.setUpToDateWithFilesystem(std::filesystem::last_write_time(*maybePath));

        App::singleton<RecentFiles>()->push_back(*maybePath);
    }
}

void osc::ActionNewModel(const ParentPtr<IMainUIStateAPI>& api)
{
    auto p = std::make_unique<UndoableModelStatePair>();
    api->add_and_select_tab<ModelEditorTab>(api, std::move(p));
}

void osc::ActionOpenModel(const ParentPtr<IMainUIStateAPI>& api)
{
    DoOpenFileViaDialog(api);
}

void osc::ActionOpenModel(const ParentPtr<IMainUIStateAPI>& api, const std::filesystem::path& path)
{
    OpenOsimInLoadingTab(api, path);
}

bool osc::ActionSaveModel(IMainUIStateAPI&, UndoableModelStatePair& model)
{
    const std::optional<std::string> maybeUserSaveLoc = TryGetModelSaveLocation(model.getModel());

    if (maybeUserSaveLoc && TrySaveModel(model.getModel(), *maybeUserSaveLoc))
    {
        const std::string oldPath = model.getModel().getInputFileName();
        model.updModel().setInputFileName(*maybeUserSaveLoc);
        model.setFilesystemPath(*maybeUserSaveLoc);

        if (*maybeUserSaveLoc != oldPath)
        {
            model.commit("changed osim path");
        }
        model.setUpToDateWithFilesystem(std::filesystem::last_write_time(*maybeUserSaveLoc));

        App::singleton<RecentFiles>()->push_back(*maybeUserSaveLoc);
        return true;
    }
    else
    {
        return false;
    }
}

void osc::ActionTryDeleteSelectionFromEditedModel(UndoableModelStatePair& uim)
{
    const OpenSim::Component* const selected = uim.getSelected();

    if (!selected)
    {
        return;
    }

    const OpenSim::ComponentPath selectedPath = GetAbsolutePath(*selected);

    const UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::Component* const mutComponent = FindComponentMut(mutModel, selectedPath);

    if (!mutComponent)
    {
        uim.setModelVersion(oldVersion);
        return;
    }

    const std::string selectedComponentName = mutComponent->getName();

    if (TryDeleteComponentFromModel(mutModel, *mutComponent))
    {
        try
        {
            InitializeModel(mutModel);
            InitializeState(mutModel);

            std::stringstream ss;
            ss << "deleted " << selectedComponentName;
            uim.commit(std::move(ss).str());
        }
        catch (const std::exception& ex)
        {
            log_error("error detected while deleting a component: %s", ex.what());
            uim.rollback();
        }
    }
    else
    {
        uim.setModelVersion(oldVersion);
    }
}

void osc::ActionUndoCurrentlyEditedModel(UndoableModelStatePair& model)
{
    if (model.canUndo())
    {
        model.doUndo();
    }
}

void osc::ActionRedoCurrentlyEditedModel(UndoableModelStatePair& model)
{
    if (model.canRedo())
    {
        model.doRedo();
    }
}

void osc::ActionDisableAllWrappingSurfaces(UndoableModelStatePair& model)
{
    try
    {
        OpenSim::Model& mutModel = model.updModel();
        DeactivateAllWrapObjectsIn(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.commit("disabled all wrapping surfaces");
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while disabling wrapping surfaces: %s", ex.what());
        model.rollback();
    }
}

void osc::ActionEnableAllWrappingSurfaces(UndoableModelStatePair& model)
{
    try
    {
        OpenSim::Model& mutModel = model.updModel();
        ActivateAllWrapObjectsIn(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.commit("enabled all wrapping surfaces");
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while enabling wrapping surfaces: %s", ex.what());
        model.rollback();
    }
}

void osc::ActionClearSelectionFromEditedModel(UndoableModelStatePair& model)
{
    model.setSelected(nullptr);
}

bool osc::ActionLoadSTOFileAgainstModel(
    const ParentPtr<IMainUIStateAPI>& parent,
    const UndoableModelStatePair& uim,
    const std::filesystem::path& stoPath)
{
    try
    {
        auto modelCopy = std::make_unique<OpenSim::Model>(uim.getModel());
        InitializeModel(*modelCopy);
        InitializeState(*modelCopy);

        auto simulation = std::make_shared<Simulation>(StoFileSimulation{std::move(modelCopy), stoPath, uim.getFixupScaleFactor()});

        parent->add_and_select_tab<SimulationTab>(parent, simulation);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to load an STO file against the model: %s", ex.what());
        return false;
    }
}

bool osc::ActionStartSimulatingModel(
    const ParentPtr<IMainUIStateAPI>& parent,
    const UndoableModelStatePair& uim)
{
    BasicModelStatePair modelState{uim};
    ForwardDynamicSimulatorParams params = FromParamBlock(parent->getSimulationParams());

    auto simulation = std::make_shared<Simulation>(ForwardDynamicSimulation{std::move(modelState), params});
    auto simulationTab = std::make_unique<SimulationTab>(parent, std::move(simulation));

    parent->select_tab(parent->add_tab(std::move(simulationTab)));

    return true;
}

bool osc::ActionUpdateModelFromBackingFile(UndoableModelStatePair& uim)
{
    if (!uim.hasFilesystemLocation())
    {
        // there is no backing file?
        return false;
    }

    const std::filesystem::path path = uim.getFilesystemPath();

    if (!std::filesystem::exists(path))
    {
        // the file does not exist? (e.g. because the user deleted it externally - #495)
        return false;
    }

    const std::filesystem::file_time_type lastSaveTime = std::filesystem::last_write_time(path);

    if (uim.getLastFilesystemWriteTime() >= lastSaveTime)
    {
        // the backing file is probably up-to-date with the in-memory representation
        //
        // (e.g. because OSC just saved it and set the timestamp appropriately)
        return false;
    }

    // else: there is a backing file and it's newer than what's in-memory, so reload
    try
    {
        log_info("file change detected: loading updated file");

        auto loadedModel = std::make_unique<OpenSim::Model>(uim.getModel().getInputFileName());

        log_info("loaded updated file");

        uim.setModel(std::move(loadedModel));
        uim.commit("reloaded osim");
        uim.setUpToDateWithFilesystem(lastSaveTime);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to automatically load a model file: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionCopyModelPathToClipboard(const UndoableModelStatePair& uim)
{
    if (!uim.hasFilesystemLocation())
    {
        // there is no backing file?
        return false;
    }

    const std::filesystem::path absPath = std::filesystem::weakly_canonical(uim.getFilesystemPath());

    set_clipboard_text(absPath.string());

    return true;
}

bool osc::ActionAutoscaleSceneScaleFactor(UndoableModelStatePair& uim)
{
    const float sf = GetRecommendedScaleFactor(
        *App::singleton<SceneCache>(App::resource_loader()),
        uim.getModel(),
        uim.getState(),
        OpenSimDecorationOptions{}
    );
    uim.setFixupScaleFactor(sf);
    return true;
}

bool osc::ActionToggleFrames(UndoableModelStatePair& uim)
{
    try
    {
        OpenSim::Model& mutModel = uim.updModel();
        const bool newState = ToggleShowingFrames(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit(newState ? "shown frames" : "hidden frames");

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to toggle frames: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionToggleMarkers(UndoableModelStatePair& uim)
{
    try
    {
        OpenSim::Model& mutModel = uim.updModel();
        const bool newState = ToggleShowingMarkers(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit(newState ? "shown markers" : "hidden markers");

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to toggle markers: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionToggleContactGeometry(UndoableModelStatePair& uim)
{
    try
    {
        OpenSim::Model& mutModel = uim.updModel();
        const bool newState = ToggleShowingContactGeometry(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit(newState ? "shown contact geometry" : "hidden contact geometry");

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to toggle contact geometry: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionToggleWrapGeometry(UndoableModelStatePair& uim)
{
    try
    {
        OpenSim::Model& mutModel = uim.updModel();
        const bool newState = ToggleShowingWrapGeometry(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit(newState ? "shown wrap geometry" : "hidden wrap geometry");

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to toggle wrap geometry: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionOpenOsimParentDirectory(UndoableModelStatePair& uim)
{
    if (!HasInputFileName(uim.getModel()))
    {
        return false;
    }

    const std::filesystem::path p{uim.getModel().getInputFileName()};
    open_file_in_os_default_application(p.parent_path());
    return true;
}

bool osc::ActionOpenOsimInExternalEditor(UndoableModelStatePair& uim)
{
    if (!HasInputFileName(uim.getModel()))
    {
        return false;
    }

    open_file_in_os_default_application(uim.getModel().getInputFileName());
    return true;
}

bool osc::ActionReloadOsimFromDisk(UndoableModelStatePair& uim, SceneCache& meshCache)
{
    if (!HasInputFileName(uim.getModel()))
    {
        log_error("cannot reload the osim file: the model doesn't appear to have a backing file (is it saved?)");
        return false;
    }

    try
    {
        log_info("manual osim file reload requested: attempting to reload the file");
        auto p = std::make_unique<OpenSim::Model>(uim.getModel().getInputFileName());
        log_info("loaded updated file");

        uim.setModel(std::move(p));
        uim.commit("reloaded from filesystem");
        uim.setUpToDateWithFilesystem(std::filesystem::last_write_time(uim.getFilesystemPath()));

        // #594: purge the app-wide mesh cache so that any user edits to the underlying
        // mesh files are immediately visible after reloading
        //
        // this is useful for users that are actively editing the meshes of the model file
        meshCache.clear_meshes();

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to reload a model file: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionSimulateAgainstAllIntegrators(
    const ParentPtr<IMainUIStateAPI>& parent,
    const UndoableModelStatePair& uim)
{
    parent->add_and_select_tab<PerformanceAnalyzerTab>(
        parent,
        BasicModelStatePair{uim},
        parent->getSimulationParams()
    );
    return true;
}

bool osc::ActionAddOffsetFrameToPhysicalFrame(UndoableModelStatePair& uim, const OpenSim::ComponentPath& path)
{
    const auto* const target = FindComponent<OpenSim::PhysicalFrame>(uim.getModel(), path);
    if (!target)
    {
        return false;
    }

    const std::string newPofName = target->getName() + "_offsetframe";

    auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pof->setName(newPofName);
    pof->setParentFrame(*target);

    const UID oldVersion = uim.getModelVersion();  // for rollbacks
    try
    {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutTarget = FindComponentMut<OpenSim::PhysicalFrame>(mutModel, path);
        if (!mutTarget)
        {
            uim.setModelVersion(oldVersion);
            return false;
        }

        OpenSim::PhysicalOffsetFrame& pofRef = AddComponent(*mutTarget, std::move(pof));
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.setSelected(&pofRef);
        uim.commit("added " + newPofName);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to add a frame to %s: %s", path.toString().c_str(), ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::CanRezeroJoint(UndoableModelStatePair& uim, const OpenSim::ComponentPath& jointPath)
{
    const auto* const joint = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (!joint)
    {
        return false;
    }

    // if the joint uses offset frames for both its parent and child frames then
    // it is possible to reorient those frames such that the joint's new zero
    // point is whatever the current arrangement is (effectively, by pre-transforming
    // the parent into the child and assuming a "zeroed" joint is an identity op)

    return dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&joint->getParentFrame()) != nullptr;
}

bool osc::ActionRezeroJoint(UndoableModelStatePair& uim, const OpenSim::ComponentPath& jointPath)
{
    const auto* const target = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (!target)
    {
        return false;  // nothing/invalid component type specified
    }

    const auto* const parentPOF = dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&target->getParentFrame());
    if (!parentPOF)
    {
        return false;  // target has no parent frame
    }

    const OpenSim::ComponentPath parentPath = GetAbsolutePath(*parentPOF);
    const OpenSim::PhysicalFrame& childFrame = target->getChildFrame();
    const SimTK::Transform parentXform = parentPOF->getTransformInGround(uim.getState());
    const SimTK::Transform childXform = childFrame.getTransformInGround(uim.getState());
    const SimTK::Transform child2parent = parentXform.invert() * childXform;
    const SimTK::Transform newXform = parentPOF->getOffsetTransform() * child2parent;

    const UID oldVersion = uim.getModelVersion();  // for rollbacks
    try
    {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutJoint = FindComponentMut<OpenSim::Joint>(mutModel, jointPath);
        if (!mutJoint)
        {
            uim.setModelVersion(oldVersion);  // cannot find mutable version of the joint
            return false;
        }

        auto* const mutParent = FindComponentMut<OpenSim::PhysicalOffsetFrame>(mutModel, parentPath);
        if (!mutParent)
        {
            uim.setModelVersion(oldVersion);  // cannot find mutable version of the parent offset frame
            return false;
        }

        // else: perform model transformation

        const std::string jointName = mutJoint->getName();

        // first, zero all the joint's coordinates
        //
        // (we're assuming that the new transform performs the same function)
        for (int i = 0, nc = mutJoint->getProperty_coordinates().size(); i < nc; ++i)
        {
            mutJoint->upd_coordinates(i).setDefaultValue(0.0);
        }

        // then set the parent offset frame's transform to "do the work"
        mutParent->setOffsetTransform(newXform);

        // and then put the model back into a valid state, ready for committing etc.
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit("rezeroed " + jointName);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to rezero a joint: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionAddParentOffsetFrameToJoint(UndoableModelStatePair& uim, const OpenSim::ComponentPath& jointPath)
{
    const auto* const target = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (!target)
    {
        return false;
    }

    auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pf->setParentFrame(target->getParentFrame());

    const UID oldVersion = uim.getModelVersion();  // for rollbacks
    try
    {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutJoint = FindComponentMut<OpenSim::Joint>(mutModel, jointPath);
        if (!mutJoint)
        {
            uim.setModelVersion(oldVersion);
            return false;
        }

        const std::string jointName = mutJoint->getName();

        mutJoint->connectSocket_parent_frame(*pf);
        AddFrame(*mutJoint, std::move(pf));
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit("added " + jointName);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to add a parent offset frame: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionAddChildOffsetFrameToJoint(UndoableModelStatePair& uim, const OpenSim::ComponentPath& jointPath)
{
    const auto* const target = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (!target)
    {
        return false;
    }

    auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pf->setParentFrame(target->getChildFrame());

    const UID oldVersion = uim.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutJoint = FindComponentMut<OpenSim::Joint>(mutModel, jointPath);
        if (!mutJoint)
        {
            uim.setModelVersion(oldVersion);
            return false;
        }

        const std::string jointName = mutJoint->getName();

        mutJoint->connectSocket_child_frame(*pf);
        AddFrame(*mutJoint, std::move(pf));
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit("added " + jointName);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to add a child offset frame: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionSetComponentName(UndoableModelStatePair& uim, const OpenSim::ComponentPath& path, const std::string& newName)
{
    if (newName.empty())
    {
        return false;
    }

    const OpenSim::Component* const target = FindComponent(uim.getModel(), path);
    if (!target)
    {
        return false;
    }

    const UID oldVersion = uim.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = uim.updModel();

        OpenSim::Component* const mutComponent = FindComponentMut(mutModel, path);
        if (!mutComponent)
        {
            uim.setModelVersion(oldVersion);
            return false;
        }

        const std::string oldName = mutComponent->getName();
        mutComponent->setName(newName);
        FinalizeConnections(mutModel);  // because pointers need to know the new name
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.setSelected(mutComponent);  // because the name changed

        std::stringstream ss;
        ss << "renamed " << oldName << " to " << newName;
        uim.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to set a component's name: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionChangeJointTypeTo(UndoableModelStatePair& uim, const OpenSim::ComponentPath& jointPath, std::unique_ptr<OpenSim::Joint> newType)
{
    if (!newType)
    {
        log_error("new joint type provided to ChangeJointType function is nullptr: cannot continue: this is a developer error and should be reported");
        return false;
    }

    const auto* const target = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (!target)
    {
        log_error("could not find %s in the model", jointPath.toString().c_str());
        return false;
    }

    const auto* const owner = GetOwner<OpenSim::JointSet>(*target);
    if (!owner)
    {
        log_error("%s is not owned by an OpenSim::JointSet", jointPath.toString().c_str());
        return false;
    }

    const OpenSim::ComponentPath ownerPath = GetAbsolutePath(*owner);

    const std::optional<size_t> maybeIdx = FindJointInParentJointSet(*target);
    if (!maybeIdx)
    {
        log_error("%s could not be found in its owner", jointPath.toString().c_str());
        return false;
    }

    const size_t idx = *maybeIdx;

    const std::string oldTypeName = target->getConcreteClassName();
    const std::string newTypeName = newType->getConcreteClassName();

    CopyCommonJointProperties(*target, *newType);

    // perform model update by overwriting the old joint in model
    //
    // note: this will invalidate the input joint, because the
    // OpenSim::JointSet container will automatically kill it

    const UID oldVersion = uim.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutParent = FindComponentMut<OpenSim::JointSet>(mutModel, ownerPath);
        if (!mutParent)
        {
            uim.setModelVersion(oldVersion);
            return false;
        }

        const OpenSim::Joint& jointRef = Assign(*mutParent, idx, std::move(newType));
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.setSelected(&jointRef);

        std::stringstream ss;
        ss << "changed " << oldTypeName << " to " << newTypeName;
        uim.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to change a joint's type: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionAttachGeometryToPhysicalFrame(UndoableModelStatePair& uim, const OpenSim::ComponentPath& physFramePath, std::unique_ptr<OpenSim::Geometry> geom)
{
    const auto* const target = FindComponent<OpenSim::PhysicalFrame>(uim.getModel(), physFramePath);
    if (!target)
    {
        return false;
    }

    const UID oldVersion = uim.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutPof = FindComponentMut<OpenSim::PhysicalFrame>(mutModel, physFramePath);
        if (!mutPof)
        {
            uim.setModelVersion(oldVersion);
            return false;
        }

        const std::string pofName = mutPof->getName();

        AttachGeometry(*mutPof, std::move(geom));
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "attached geometry to " << pofName;
        uim.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to attach geometry to the a physical frame: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionAssignContactGeometryToHCF(
    UndoableModelStatePair& uim,
    const OpenSim::ComponentPath& hcfPath,
    const OpenSim::ComponentPath& contactGeomPath)
{
    const auto* const target = FindComponent<OpenSim::HuntCrossleyForce>(uim.getModel(), hcfPath);
    if (!target)
    {
        return false;
    }

    const auto* const geom = FindComponent<OpenSim::ContactGeometry>(uim.getModel(), contactGeomPath);
    if (!geom)
    {
        return false;
    }

    const UID oldVersion = uim.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutHCF = FindComponentMut<OpenSim::HuntCrossleyForce>(mutModel, hcfPath);
        if (!mutHCF)
        {
            uim.setModelVersion(oldVersion);
            return false;
        }

        // calling this ensures at least one `OpenSim::HuntCrossleyForce::ContactParameters`
        // is present in the HCF
        mutHCF->getStaticFriction();
        OSC_ASSERT(!empty(mutHCF->updContactParametersSet()));

        mutHCF->updContactParametersSet()[0].updGeometry().appendValue(geom->getName());
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit("added contact geometry");

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to assign contact geometry to a HCF: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionApplyPropertyEdit(UndoableModelStatePair& uim, ObjectPropertyEdit& resp)
{
    const UID oldVersion = uim.getModelVersion();
    try
    {
        OpenSim::Model& model = uim.updModel();

        OpenSim::Component* const component = FindComponentMut(model, resp.getComponentAbsPath());
        if (!component)
        {
            uim.setModelVersion(oldVersion);
            return false;
        }

        OpenSim::AbstractProperty* const prop = FindPropertyMut(*component, resp.getPropertyName());
        if (!prop)
        {
            uim.setModelVersion(oldVersion);
            return false;
        }

        const std::string propName = prop->getName();

        resp.apply(*prop);

        const std::string newValue = prop->toStringForDisplay(3);

        InitializeModel(model);
        InitializeState(model);

        std::stringstream ss;
        ss << "set " << propName << " to " << newValue;
        uim.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to apply a property edit: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionAddPathPointToPathActuator(
    UndoableModelStatePair& uim,
    const OpenSim::ComponentPath& pathActuatorPath,
    const OpenSim::ComponentPath& pointPhysFrame)
{
    const auto* const pa = FindComponent<OpenSim::PathActuator>(uim.getModel(), pathActuatorPath);
    if (!pa)
    {
        return false;
    }

    const auto* const pf = FindComponent<OpenSim::PhysicalFrame>(uim.getModel(), pointPhysFrame);
    if (!pf)
    {
        return false;
    }

    const size_t n = size(pa->getGeometryPath().getPathPointSet());
    const std::string name = pa->getName() + "-P" + std::to_string(n + 1);
    const SimTK::Vec3 pos = {0.0f, 0.0f, 0.0f};

    const UID oldVersion = uim.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutPA = FindComponentMut<OpenSim::PathActuator>(mutModel, pathActuatorPath);
        if (!mutPA)
        {
            uim.setModelVersion(oldVersion);
            return false;
        }

        const std::string paName = mutPA->getName();

        mutPA->addNewPathPoint(name, *pf, pos);
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);

        // try to select the new path point, if possible, so that the user
        // can immediately see the grab handles etc. (#779)
        if (const auto* paAfterFinalization = FindComponent<OpenSim::PathActuator>(mutModel, pathActuatorPath))
        {
            const auto& pps = paAfterFinalization->getGeometryPath().getPathPointSet();
            if (!empty(pps))
            {
                uim.setSelected(&At(pps, ssize(pps) -1));
            }
        }

        std::stringstream ss;
        ss << "added path point to " << paName;
        uim.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to add a path point to a path actuator: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionReassignComponentSocket(
    UndoableModelStatePair& uim,
    const OpenSim::ComponentPath& componentAbsPath,
    const std::string& socketName,
    const OpenSim::Object& connectee,
    SocketReassignmentFlags flags,
    std::string& error)
{
    // HOTFIX for #382
    //
    // OpenSim can segfault if certain types of circular joint connections to `/ground` are made.
    // This early-out error just ensures that OpenSim Creator isn't nuked by that OpenSim bug
    //
    // issue #3299 in opensim-core
    if (socketName == "child_frame" && &connectee == &uim.getModel().getGround())
    {
        error = "Error: you cannot assign a joint's child frame to ground: this is a known bug in OpenSim (see issue #382 in ComputationalBiomechanicsLab/opensim-creator and issue #3299 in opensim-org/opensim-core)";
        return false;
    }

    const OpenSim::Component* const target = FindComponent(uim.getModel(), componentAbsPath);
    if (!target)
    {
        return false;
    }

    const UID oldVersion = uim.getModelVersion();

    OpenSim::Model& mutModel = uim.updModel();

    OpenSim::Component* const mutComponent = FindComponentMut(mutModel, componentAbsPath);
    if (!mutComponent)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    OpenSim::AbstractSocket* const mutSocket = FindSocketMut(*mutComponent, socketName);
    if (!mutSocket)
    {
        uim.setModelVersion(oldVersion);
        return false;
    }

    try
    {
        const bool componentPropertiesReexpressed = flags & SocketReassignmentFlags::TryReexpressComponentInNewConnectee ?
            TryReexpressComponentSpatialPropertiesInNewConnectee(*mutComponent, connectee, uim.getState()) :
            false;

        if (componentPropertiesReexpressed)
        {
            FinalizeFromProperties(mutModel);
        }
        mutSocket->connect(connectee);
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit("reassigned socket");

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to reassign a socket: %s", ex.what());
        error = ex.what();
        uim.rollback();
        return false;
    }
}

bool osc::ActionSetModelSceneScaleFactorTo(UndoableModelStatePair& uim, float v)
{
    uim.setFixupScaleFactor(v);
    return true;
}

osc::BodyDetails::BodyDetails() :
    centerOfMass{0.0f, 0.0f, 0.0f},
    inertia{1.0f, 1.0f, 1.0f},
    mass{1.0f},
    bodyName{"new_body"},
    jointTypeIndex{IndexOf<OpenSim::WeldJoint>(GetComponentRegistry<OpenSim::Joint>()).value_or(0)},
    maybeGeometry{nullptr},
    addOffsetFrames{true}
{
}

bool osc::ActionAddBodyToModel(UndoableModelStatePair& uim, const BodyDetails& details)
{
    const auto* const parent = FindComponent<OpenSim::PhysicalFrame>(uim.getModel(), details.parentFrameAbsPath);
    if (!parent)
    {
        return false;
    }

    const SimTK::Vec3 com = ToSimTKVec3(details.centerOfMass);
    const SimTK::Inertia inertia = ToSimTKInertia(details.inertia);
    const auto mass = static_cast<double>(details.mass);

    // create body
    auto body = std::make_unique<OpenSim::Body>(details.bodyName, mass, com, inertia);

    // create joint between body and whatever the frame is
    const OpenSim::Joint& jointProto = At(GetComponentRegistry<OpenSim::Joint>(), details.jointTypeIndex).prototype();
    std::unique_ptr<OpenSim::Joint> joint = MakeJoint(details, *body, jointProto, *parent);

    // attach decorative geom
    if (details.maybeGeometry)
    {
        AttachGeometry(*body, Clone(*details.maybeGeometry));
    }

    // mutate the model and perform the edit
    try
    {
        OpenSim::Model& mutModel = uim.updModel();

        AddJoint(mutModel, std::move(joint));
        OpenSim::Body& bodyRef = AddBody(mutModel, std::move(body));
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.setSelected(&bodyRef);

        std::stringstream ss;
        ss << "added " << bodyRef.getName();
        uim.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to add a body to the model: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionAddComponentToModel(UndoableModelStatePair& model, std::unique_ptr<OpenSim::Component> c, std::string& errorOut)
{
    if (c == nullptr)
    {
        return false;  // paranoia
    }

    try
    {
        OpenSim::Model& mutModel = model.updModel();

        const OpenSim::Component& ref = AddComponentToAppropriateSet(mutModel, std::move(c));
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.setSelected(&ref);

        std::stringstream ss;
        ss << "added " << ref.getName();
        model.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception& ex)
    {
        errorOut = ex.what();
        log_error("error detected while trying to add a component to the model: %s", ex.what());
        model.rollback();
        return false;
    }
}

bool osc::ActionAddWrapObjectToPhysicalFrame(
    UndoableModelStatePair& model,
    const OpenSim::ComponentPath& physicalFramePath,
    std::unique_ptr<OpenSim::WrapObject> wrapObjPtr)
{
    OSC_ASSERT(wrapObjPtr != nullptr);

    if (!FindComponent<OpenSim::PhysicalFrame>(model.getModel(), physicalFramePath)) {
        return false;  // cannot find the `OpenSim::PhysicalFrame` in the model
    }

    try {
        OpenSim::Model& mutModel = model.updModel();
        auto* frame = FindComponentMut<OpenSim::PhysicalFrame>(mutModel, physicalFramePath);
        OSC_ASSERT_ALWAYS(frame != nullptr && "cannot find the given OpenSim::PhysicalFrame in the model");

        OpenSim::WrapObject& wrapObj = AddWrapObject(*frame, std::move(wrapObjPtr));
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.setSelected(&wrapObj);

        std::stringstream ss;
        ss << "added " << wrapObj.getName();
        model.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception& ex) {
        log_error("error detected while trying to add a wrap object to the model: %s", ex.what());
        model.rollback();
        return false;
    }
}

bool osc::ActionAddWrapObjectToGeometryPathWraps(
    UndoableModelStatePair& model,
    const OpenSim::GeometryPath& geomPath,
    const OpenSim::WrapObject& wrapObject)
{
    try {
        OpenSim::Model& mutModel = model.updModel();
        auto* mutGeomPath = FindComponentMut<OpenSim::GeometryPath>(mutModel, geomPath.getAbsolutePath());
        OSC_ASSERT_ALWAYS(mutGeomPath != nullptr && "cannot find the geometry path in the model");
        auto* mutWrapObject = FindComponentMut<OpenSim::WrapObject>(mutModel, wrapObject.getAbsolutePath());
        OSC_ASSERT_ALWAYS(mutWrapObject != nullptr && "cannot find wrap object in the model");

        std::stringstream msg;
        msg << "added " << mutWrapObject->getName() << " to " << mutGeomPath->getName();

        mutGeomPath->addPathWrap(*mutWrapObject);
        InitializeModel(mutModel);
        InitializeState(mutModel);

        model.commit(std::move(msg).str());
        return true;
    }
    catch (const std::exception& ex) {
        log_error("error detected while trying to add a wrap object to a geometry path: %s", ex.what());
        model.rollback();
        return false;
    }
}

bool osc::ActionRemoveWrapObjectFromGeometryPathWraps(
    UndoableModelStatePair& model,
    const OpenSim::GeometryPath& geomPath,
    const OpenSim::WrapObject& wrapObject)
{
    // search for the wrap object in the geometry path's wrap list
    std::optional<int> index;
    for (int i = 0; i < geomPath.getWrapSet().getSize(); ++i) {
        if (geomPath.getWrapSet().get(i).getWrapObject() == &wrapObject) {
            index = i;
            break;
        }
    }

    if (!index) {
        log_info("cannot find the %s in %s: skipping deletion", wrapObject.getName().c_str(), geomPath.getName().c_str());
        return false;
    }

    try {
        OpenSim::Model& mutModel = model.updModel();
        auto* mutGeomPath = FindComponentMut<OpenSim::GeometryPath>(mutModel, geomPath.getAbsolutePath());
        OSC_ASSERT_ALWAYS(mutGeomPath != nullptr && "cannot find the geometry path in the model");
        auto* mutWrapObject = FindComponentMut<OpenSim::WrapObject>(mutModel, wrapObject.getAbsolutePath());
        OSC_ASSERT_ALWAYS(mutWrapObject != nullptr && "cannot find wrap object in the model");

        std::stringstream msg;
        msg << "removed " << mutWrapObject->getName() << " from " << mutGeomPath->getName();

        OSC_ASSERT(index);
        mutGeomPath->deletePathWrap(model.getState(), *index);
        InitializeModel(mutModel);
        InitializeState(mutModel);

        model.commit(std::move(msg).str());
        return true;
    }
    catch (const std::exception& ex) {
        log_error("error detected while trying to add a wrap object to a geometry path: %s", ex.what());
        model.rollback();
        return false;
    }
}

bool osc::ActionSetCoordinateSpeed(
    UndoableModelStatePair& model,
    const OpenSim::Coordinate& coord,
    double newSpeed)
{
    const OpenSim::ComponentPath coordPath = GetAbsolutePath(coord);

    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutCoord = FindComponentMut<OpenSim::Coordinate>(mutModel, coordPath);
        if (!mutCoord)
        {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        // HACK: don't do a full model+state re-realization here: only do it
        //       when the caller wants to save the coordinate change
        mutCoord->setDefaultSpeedValue(newSpeed);
        mutCoord->setSpeedValue(mutModel.updWorkingState(), newSpeed);
        mutModel.equilibrateMuscles(mutModel.updWorkingState());
        mutModel.realizeDynamics(mutModel.updWorkingState());

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to set a coordinate's speed: %s", ex.what());
        model.rollback();
        return false;
    }
}

bool osc::ActionSetCoordinateSpeedAndSave(
    UndoableModelStatePair& model,
    const OpenSim::Coordinate& coord,
    double newSpeed)
{
    if (ActionSetCoordinateSpeed(model, coord, newSpeed))
    {
        OpenSim::Model& mutModel = model.updModel();
        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "set " << coord.getName() << "'s speed";
        model.commit(std::move(ss).str());

        return true;
    }
    else
    {
        // edit wasn't made
        return false;
    }
}

bool osc::ActionSetCoordinateLockedAndSave(UndoableModelStatePair& model, const OpenSim::Coordinate& coord, bool v)
{
    const OpenSim::ComponentPath coordPath = GetAbsolutePath(coord);

    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutCoord = FindComponentMut<OpenSim::Coordinate>(mutModel, coordPath);
        if (!mutCoord)
        {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        mutCoord->setDefaultLocked(v);
        mutCoord->setLocked(mutModel.updWorkingState(), v);
        mutModel.equilibrateMuscles(mutModel.updWorkingState());
        mutModel.realizeDynamics(mutModel.updWorkingState());

        std::stringstream ss;
        ss << (v ? "locked " : "unlocked ") << mutCoord->getName();
        model.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to lock a coordinate: %s", ex.what());
        model.rollback();
        return false;
    }
}

// set the value of a coordinate, but don't save it to the model (yet)
bool osc::ActionSetCoordinateValue(
    UndoableModelStatePair& model,
    const OpenSim::Coordinate& coord,
    double newValue)
{
    const OpenSim::ComponentPath coordPath = GetAbsolutePath(coord);

    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutCoord = FindComponentMut<OpenSim::Coordinate>(mutModel, coordPath);
        if (!mutCoord)
        {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        const double rangeMin = min(mutCoord->getRangeMin(), mutCoord->getRangeMax());
        const double rangeMax = max(mutCoord->getRangeMin(), mutCoord->getRangeMax());

        if (!(rangeMin <= newValue && newValue <= rangeMax))
        {
            model.setModelVersion(oldVersion);  // the requested edit is outside the coordinate's allowed range
            return false;
        }

        // HACK: don't do a full model+state re-realization here: only do it
        //       when the caller wants to save the coordinate change
        mutCoord->setDefaultValue(newValue);
        mutCoord->setValue(mutModel.updWorkingState(), newValue);
        mutModel.equilibrateMuscles(mutModel.updWorkingState());
        mutModel.realizeDynamics(mutModel.updWorkingState());

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to set a coordinate's value: %s", ex.what());
        model.rollback();
        return false;
    }
}

// set the value of a coordinate and ensure it is saved into the model
bool osc::ActionSetCoordinateValueAndSave(
    UndoableModelStatePair& model,
    const OpenSim::Coordinate& coord,
    double newValue)
{
    if (ActionSetCoordinateValue(model, coord, newValue))
    {
        OpenSim::Model& mutModel = model.updModel();

        // CAREFUL: ensure that *all* coordinate's default values are updated to reflect
        //          the current state
        //
        // You might be thinking "but, the caller only wanted to set one coordinate". You're
        // right, but OpenSim models can contain constraints where editing one coordinate causes
        // a bunch of other coordinates to change.
        //
        // See #345 for a longer explanation
        for (OpenSim::Coordinate& c : mutModel.updComponentList<OpenSim::Coordinate>())
        {
            c.setDefaultValue(c.getValue(model.getState()));
        }

        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "set " << coord.getName() << " to " << ConvertCoordValueToDisplayValue(coord, newValue);
        model.commit(std::move(ss).str());

        return true;
    }
    else
    {
        return false;  // an edit wasn't made
    }
}

bool osc::ActionSetComponentAndAllChildrensIsVisibleTo(
    UndoableModelStatePair& model,
    const OpenSim::ComponentPath& path,
    bool newVisibility)
{
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        OpenSim::Component* const mutComponent = FindComponentMut(mutModel, path);
        if (!mutComponent)
        {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        TrySetAppearancePropertyIsVisibleTo(*mutComponent, newVisibility);

        for (OpenSim::Component& c : mutComponent->updComponentList())
        {
            TrySetAppearancePropertyIsVisibleTo(c, newVisibility);
        }

        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "set " << path.getComponentName() << " visibility to " << newVisibility;
        model.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to hide a component: %s", ex.what());
        model.rollback();
        return false;
    }
}

bool osc::ActionShowOnlyComponentAndAllChildren(UndoableModelStatePair& model, const OpenSim::ComponentPath& path)
{
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        OpenSim::Component* const mutComponent = FindComponentMut(mutModel, path);
        if (!mutComponent)
        {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        // first, hide everything in the model
        for (OpenSim::Component& c : mutModel.updComponentList())
        {
            TrySetAppearancePropertyIsVisibleTo(c, false);
        }

        // then show the intended component and its children
        TrySetAppearancePropertyIsVisibleTo(*mutComponent, true);
        for (OpenSim::Component& c : mutComponent->updComponentList())
        {
            TrySetAppearancePropertyIsVisibleTo(c, true);
        }

        // reinitialize etc.
        InitializeModel(mutModel);
        InitializeState(mutModel);

        // commit it
        {
            std::stringstream ss;
            ss << "showing only " << path.getComponentName();
            model.commit(std::move(ss).str());
        }

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to hide a component: %s", ex.what());
        model.rollback();
        return false;
    }
}

bool osc::ActionSetComponentAndAllChildrenWithGivenConcreteClassNameIsVisibleTo(
    UndoableModelStatePair& model,
    const OpenSim::ComponentPath& root,
    std::string_view concreteClassName,
    bool newVisibility)
{
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        OpenSim::Component* const mutComponent = FindComponentMut(mutModel, root);
        if (!mutComponent)
        {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        // first, hide everything in the model
        for (OpenSim::Component& c : mutModel.updComponentList())
        {
            if (c.getConcreteClassName() == concreteClassName)
            {
                TrySetAppearancePropertyIsVisibleTo(c, newVisibility);
                for (OpenSim::Component& child : c.updComponentList())
                {
                    TrySetAppearancePropertyIsVisibleTo(child, newVisibility);
                }
            }
        }

        // reinitialize etc.
        InitializeModel(mutModel);
        InitializeState(mutModel);

        // commit it
        {
            std::stringstream ss;
            if (newVisibility)
            {
                ss << "showing ";
            }
            else
            {
                ss << "hiding ";
            }
            ss << concreteClassName;
            model.commit(std::move(ss).str());
        }

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to show/hide components of a given type: %s", ex.what());
        model.rollback();
        return false;
    }
}

bool osc::ActionTranslateStation(
    UndoableModelStatePair& model,
    const OpenSim::Station& station,
    const Vec3& deltaPosition)
{
    const OpenSim::ComponentPath stationPath = GetAbsolutePath(station);
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutStation = FindComponentMut<OpenSim::Station>(mutModel, stationPath);
        if (!mutStation)
        {
            model.setModelVersion(oldVersion);  // the provided path isn't a station
            return false;
        }

        const SimTK::Vec3 originalPos = mutStation->get_location();
        const SimTK::Vec3 newPos = originalPos + ToSimTKVec3(deltaPosition);

        // perform mutation
        mutStation->set_location(newPos);

        // HACK: don't perform a full model reinitialization because that would be very expensive
        // and it is very likely that it isn't necessary when dragging a station
        //
        // InitializeModel(mutModel);  // don't do this
        InitializeState(mutModel);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to move a station: %s", ex.what());
        model.rollback();
        return false;
    }
}

bool osc::ActionTranslateStationAndSave(
    UndoableModelStatePair& model,
    const OpenSim::Station& station,
    const Vec3& deltaPosition)
{
    if (ActionTranslateStation(model, station, deltaPosition))
    {
        OpenSim::Model& mutModel = model.updModel();
        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "translated " << station.getName();
        model.commit(std::move(ss).str());

        return true;
    }
    else
    {
        return false;  // edit wasn't made
    }
}

bool osc::ActionTranslatePathPoint(
    UndoableModelStatePair& model,
    const OpenSim::PathPoint& pathPoint,
    const Vec3& deltaPosition)
{
    const OpenSim::ComponentPath ppPath = GetAbsolutePath(pathPoint);
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutPathPoint = FindComponentMut<OpenSim::PathPoint>(mutModel, ppPath);
        if (!mutPathPoint)
        {
            model.setModelVersion(oldVersion);  // the provided path isn't a station
            return false;
        }

        const SimTK::Vec3 originalPos = mutPathPoint->get_location();
        const SimTK::Vec3 newPos = originalPos + ToSimTKVec3(deltaPosition);

        // perform mutation
        mutPathPoint->setLocation(newPos);
        InitializeState(mutModel);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to move a station: %s", ex.what());
        model.rollback();
        return false;
    }
}

bool osc::ActionTranslatePathPointAndSave(
    UndoableModelStatePair& model,
    const OpenSim::PathPoint& pathPoint,
    const Vec3& deltaPosition)
{
    if (ActionTranslatePathPoint(model, pathPoint, deltaPosition))
    {
        OpenSim::Model& mutModel = model.updModel();
        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "translated " << pathPoint.getName();
        model.commit(std::move(ss).str());

        return true;
    }
    else
    {
        return false;  // edit wasn't made
    }
}

bool osc::ActionTransformPof(
    UndoableModelStatePair& model,
    const OpenSim::PhysicalOffsetFrame& pof,
    const Vec3& deltaTranslationInParentFrame,
    const Eulers& newPofEulers)
{
    const OpenSim::ComponentPath pofPath = GetAbsolutePath(pof);
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutPof = FindComponentMut<OpenSim::PhysicalOffsetFrame>(mutModel, pofPath);
        if (!mutPof)
        {
            model.setModelVersion(oldVersion);  // the provided path isn't a station
            return false;
        }

        const SimTK::Vec3 originalPos = mutPof->get_translation();
        const SimTK::Vec3 newPos = originalPos + ToSimTKVec3(deltaTranslationInParentFrame);

        // perform mutation
        mutPof->set_translation(newPos);
        mutPof->set_orientation(ToSimTKVec3(newPofEulers));
        InitializeModel(mutModel);
        InitializeState(mutModel);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to transform a POF: %s", ex.what());
        model.rollback();
        return false;
    }
    return false;
}

bool osc::ActionTransformWrapObject(
    UndoableModelStatePair& model,
    const OpenSim::WrapObject& wo,
    const Vec3& deltaPosition,
    const Eulers& newEulers)
{
    const OpenSim::ComponentPath pofPath = GetAbsolutePath(wo);
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutPof = FindComponentMut<OpenSim::WrapObject>(mutModel, pofPath);
        if (!mutPof)
        {
            model.setModelVersion(oldVersion);  // the provided path isn't a station
            return false;
        }

        const SimTK::Vec3 originalPos = mutPof->get_translation();
        const SimTK::Vec3 newPos = originalPos + ToSimTKVec3(deltaPosition);

        // perform mutation
        mutPof->set_translation(newPos);
        mutPof->set_xyz_body_rotation(ToSimTKVec3(newEulers));
        InitializeModel(mutModel);
        InitializeState(mutModel);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to transform a POF: %s", ex.what());
        model.rollback();
        return false;
    }
    return false;
}

bool osc::ActionTransformContactGeometry(
    UndoableModelStatePair& model,
    const OpenSim::ContactGeometry& contactGeom,
    const Vec3& deltaPosition,
    const Eulers& newEulers)
{
    const OpenSim::ComponentPath pofPath = GetAbsolutePath(contactGeom);
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutGeom = FindComponentMut<OpenSim::ContactGeometry>(mutModel, pofPath);
        if (!mutGeom)
        {
            model.setModelVersion(oldVersion);  // the provided path doesn't exist in the model
            return false;
        }

        const SimTK::Vec3 originalPos = mutGeom->get_location();
        const SimTK::Vec3 newPos = originalPos + ToSimTKVec3(deltaPosition);

        // perform mutation
        mutGeom->set_location(newPos);
        mutGeom->set_orientation(ToSimTKVec3(newEulers));
        InitializeModel(mutModel);
        InitializeState(mutModel);

        return true;
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to transform a POF: %s", ex.what());
        model.rollback();
        return false;
    }
    return false;
}

bool osc::ActionFitSphereToMesh(
    UndoableModelStatePair& model,
    const OpenSim::Mesh& openSimMesh)
{
    // fit a sphere to the mesh
    Sphere sphere;
    try
    {
        const Mesh mesh = ToOscMeshBakeScaleFactors(model.getModel(), model.getState(), openSimMesh);
        sphere = FitSphere(mesh);
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to fit a sphere to a mesh: %s", ex.what());
        return false;
    }

    // create an `OpenSim::OffsetFrame` expressed w.r.t. the same frame as the mesh that
    // places the origin-centered `OpenSim::Sphere` at the computed `origin`
    auto offsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    offsetFrame->setName("sphere_fit");
    offsetFrame->connectSocket_parent(dynamic_cast<const OpenSim::PhysicalFrame&>(openSimMesh.getFrame()));
    offsetFrame->setOffsetTransform(SimTK::Transform{ToSimTKVec3(sphere.origin)});

    // create an origin-centered `OpenSim::Sphere` geometry to visually represent the computed
    // sphere
    auto openSimSphere = std::make_unique<OpenSim::Sphere>(sphere.radius);
    openSimSphere->setName("sphere_geom");
    openSimSphere->connectSocket_frame(*offsetFrame);
    UpdAppearanceToFittedGeom(openSimSphere->upd_Appearance());

    // perform undoable model mutation
    const OpenSim::ComponentPath openSimMeshPath = GetAbsolutePath(openSimMesh);
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();
        auto* const mutOpenSimMesh = FindComponentMut<OpenSim::Mesh>(mutModel, openSimMeshPath);
        if (!mutOpenSimMesh)
        {
            model.setModelVersion(oldVersion);  // the provided path doesn't exist in the model
            return false;
        }

        const std::string sphereName = openSimSphere->getName();
        auto& pofRef = AddModelComponent(mutModel, std::move(offsetFrame));
        auto& sphereRef = AttachGeometry(pofRef, std::move(openSimSphere));

        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.setSelected(&sphereRef);

        std::stringstream ss;
        ss << "computed " << sphereName;
        model.commit(std::move(ss).str());
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to add a sphere fit to the OpenSim model: %s", ex.what());
        return false;
    }

    return true;
}

bool osc::ActionFitEllipsoidToMesh(
    UndoableModelStatePair& model,
    const OpenSim::Mesh& openSimMesh)
{
    // fit an ellipsoid to the mesh
    Ellipsoid ellipsoid;
    try
    {
        const Mesh mesh = ToOscMeshBakeScaleFactors(model.getModel(), model.getState(), openSimMesh);
        ellipsoid = FitEllipsoid(mesh);
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to fit an ellipsoid to a mesh: %s", ex.what());
        return false;
    }

    // create an `OpenSim::OffsetFrame` expressed w.r.t. the same frame as the mesh that
    // places the origin-centered `OpenSim::Ellipsoid` at the computed ellipsoid's `origin`
    // and reorients the ellipsoid's XYZ along the computed ellipsoid directions
    //
    // (OSC note: `FitEllipsoid` in OSC should yield a right-handed coordinate system)
    auto offsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    offsetFrame->setName("ellipsoid_fit");
    offsetFrame->connectSocket_parent(dynamic_cast<const OpenSim::PhysicalFrame&>(openSimMesh.getFrame()));
    {
        // compute offset transform for ellipsoid
        SimTK::Mat33 m;
        auto directions = axis_directions_of(ellipsoid);
        m.col(0) = ToSimTKVec3(directions[0]);
        m.col(1) = ToSimTKVec3(directions[1]);
        m.col(2) = ToSimTKVec3(directions[2]);
        const SimTK::Transform t{SimTK::Rotation{m}, ToSimTKVec3(ellipsoid.origin)};
        offsetFrame->setOffsetTransform(t);
    }

    // create an origin-centered `OpenSim::Ellipsoid` geometry to visually represent the computed
    // ellipsoid
    auto openSimEllipsoid = std::make_unique<OpenSim::Ellipsoid>(ellipsoid.radii[0], ellipsoid.radii[1], ellipsoid.radii[2]);
    openSimEllipsoid->setName("ellipsoid_geom");
    openSimEllipsoid->connectSocket_frame(*offsetFrame);
    UpdAppearanceToFittedGeom(openSimEllipsoid->upd_Appearance());

    // mutate the model and add the relevant components
    const OpenSim::ComponentPath openSimMeshPath = GetAbsolutePath(openSimMesh);
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();
        auto* const mutOpenSimMesh = FindComponentMut<OpenSim::Mesh>(mutModel, openSimMeshPath);
        if (!mutOpenSimMesh)
        {
            model.setModelVersion(oldVersion);  // the provided path doesn't exist in the model
            return false;
        }

        const std::string ellipsoidName = openSimEllipsoid->getName();
        auto& pofRef = AddModelComponent(mutModel, std::move(offsetFrame));
        AttachGeometry(pofRef, std::move(openSimEllipsoid));

        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.setSelected(&pofRef);

        std::stringstream ss;
        ss << "computed" << ellipsoidName;
        model.commit(std::move(ss).str());
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to add a sphere fit to the OpenSim model: %s", ex.what());
        return false;
    }

    return true;
}

bool osc::ActionFitPlaneToMesh(
    UndoableModelStatePair& model,
    const OpenSim::Mesh& openSimMesh)
{
    // fit a plane to the mesh
    Plane plane;
    try
    {
        const Mesh mesh = ToOscMeshBakeScaleFactors(model.getModel(), model.getState(), openSimMesh);
        plane = FitPlane(mesh);
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to fit a plane to a mesh: %s", ex.what());
        return false;
    }

    // create an `OpenSim::OffsetFrame` expressed w.r.t. the same frame as the mesh that
    // places the origin-centered `OpenSim::Brick` at the computed plane's `origin` and
    // also reorients the +1 in Y brick along the plane's normal
    auto offsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    offsetFrame->setName("plane_fit");
    offsetFrame->connectSocket_parent(dynamic_cast<const OpenSim::PhysicalFrame&>(openSimMesh.getFrame()));
    {
        // +1Y in "brick space" should map to the plane's normal
        const Quat q = rotation({0.0f, 1.0f, 0.0f}, plane.normal);
        offsetFrame->setOffsetTransform(SimTK::Transform{ToSimTKRotation(q), ToSimTKVec3(plane.origin)});
    }

    // create an origin-centered `OpenSim::Brick` geometry to visually represent the computed
    // plane
    auto openSimBrick = std::make_unique<OpenSim::Brick>();
    openSimBrick->set_half_lengths({0.2, 0.0005, 0.2});  // hard-coded, for now - the thin axis points along the normal
    openSimBrick->setName("plane_geom");
    openSimBrick->connectSocket_frame(*offsetFrame);
    UpdAppearanceToFittedGeom(openSimBrick->upd_Appearance());

    // mutate the model and add the relevant components
    const OpenSim::ComponentPath openSimMeshPath = GetAbsolutePath(openSimMesh);
    const UID oldVersion = model.getModelVersion();
    try
    {
        OpenSim::Model& mutModel = model.updModel();
        auto* const mutOpenSimMesh = FindComponentMut<OpenSim::Mesh>(mutModel, openSimMeshPath);
        if (!mutOpenSimMesh)
        {
            model.setModelVersion(oldVersion);  // the provided path doesn't exist in the model
            return false;
        }

        const std::string fitName = offsetFrame->getName();
        auto& pofRef = AddModelComponent(mutModel, std::move(offsetFrame));
        AttachGeometry(pofRef, std::move(openSimBrick));

        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.setSelected(&pofRef);

        std::stringstream ss;
        ss << "computed " << fitName;
        model.commit(std::move(ss).str());
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to add a sphere fit to the OpenSim model: %s", ex.what());
        return false;
    }

    return true;
}

bool osc::ActionImportLandmarks(
    UndoableModelStatePair& model,
    std::span<const lm::NamedLandmark> lms,
    std::optional<std::string> maybeName)
{
    try
    {
        OpenSim::Model& mutModel = model.updModel();
        for (const auto& lm : lms)
        {
            AddMarker(mutModel, lm.name, mutModel.getGround(), ToSimTKVec3(lm.position));
        }
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "imported " << std::move(maybeName).value_or("markers");
        model.commit(std::move(ss).str());
    }
    catch (const std::exception& ex)
    {
        log_error("error detected while trying to import landmarks to the model: %s", ex.what());
        return false;
    }
    return true;
}

bool osc::ActionExportModelGraphToDotviz(const UndoableModelStatePair& model)
{
    if (auto p = prompt_user_for_file_save_location_add_extension_if_necessary("dot")) {
        if (std::ofstream of{*p}) {
            WriteComponentTopologyGraphAsDotViz(model.getModel(), of);
            return true;
        }
        else {
            log_error("error opening %s for writing", p->string().c_str());
            return false;
        }
    }
    return false;  // user cancelled out
}

bool osc::ActionExportModelGraphToDotvizClipboard(const UndoableModelStatePair& model)
{
    std::stringstream ss;
    WriteComponentTopologyGraphAsDotViz(model.getModel(), ss);
    set_clipboard_text(std::move(ss).str());
    return true;
}

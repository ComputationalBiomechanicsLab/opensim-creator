#include "UndoableModelActions.h"

#include <libopensimcreator/ComponentRegistry/ComponentRegistry.h>
#include <libopensimcreator/ComponentRegistry/StaticComponentRegistries.h>
#include <libopensimcreator/Documents/FileFilters.h>
#include <libopensimcreator/Documents/Model/BasicModelStatePair.h>
#include <libopensimcreator/Documents/Model/Environment.h>
#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Documents/Simulation/ForwardDynamicSimulation.h>
#include <libopensimcreator/Documents/Simulation/ForwardDynamicSimulatorParams.h>
#include <libopensimcreator/Documents/Simulation/Simulation.h>
#include <libopensimcreator/Documents/Simulation/StoFileSimulation.h>
#include <libopensimcreator/Graphics/OpenSimDecorationGenerator.h>
#include <libopensimcreator/Graphics/OpenSimDecorationOptions.h>
#include <libopensimcreator/Platform/RecentFiles.h>
#include <libopensimcreator/UI/LoadingTab.h>
#include <libopensimcreator/UI/ModelEditor/ModelEditorTab.h>
#include <libopensimcreator/UI/Shared/ObjectPropertiesEditor.h>
#include <libopensimcreator/UI/Simulation/SimulationTab.h>

#include <libopynsim/Utils/OpenSimHelpers.h>
#include <libopynsim/Utils/ShapeFitters.h>
#include <libopynsim/Utils/simbody_x_oscar.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/maths/ellipsoid.h>
#include <liboscar/maths/ellipsoid_functions.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/plane.h>
#include <liboscar/maths/quaternion.h>
#include <liboscar/maths/sphere.h>
#include <liboscar/maths/vector3.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/utils/algorithms.h>
#include <liboscar/utils/assertions.h>
#include <liboscar/utils/filesystem_helpers.h>
#include <liboscar/utils/uid.h>
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
#include <OpenSim/Simulation/Model/Scholz2015GeometryPath.h>
#include <OpenSim/Simulation/Model/StationDefinedFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Coordinate.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <iterator>
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
    void OpenOsimInLoadingTab(Widget& parent, std::filesystem::path p)
    {
        auto tab = std::make_unique<LoadingTab>(&parent, std::move(p));
        App::post_event<OpenTabEvent>(parent, std::move(tab));
    }

    void DoOpenFileViaDialog(Widget& api)
    {
        App::upd().prompt_user_to_select_file_async(
            [widget_ptr = api.weak_ref()](FileDialogResponse response)  // NOLINT(performance-unnecessary-value-param)
            {
                if (not widget_ptr) {
                    return;  // widget was deleted at some point
                }

                if (response.has_error()) {
                    log_error("Error opening dialog: %s", response.error().c_str());
                    return;
                }

                for (const auto& path : response) {
                    OpenOsimInLoadingTab(*widget_ptr, path);
                }
            },
            GetModelFileFilters(),
            std::nullopt,  // initial directory
            true  // allow many
        );
    }

    bool IsAnExampleFile(const std::filesystem::path& path)
    {
        if (const auto exampleModelsDirectoryPath = App::resource_filepath("OpenSimCreator/models")) {
            return is_subpath(*exampleModelsDirectoryPath, path);
        } else {
            return false;
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

    bool TrySaveModel(const OpenSim::Model& model, const std::filesystem::path& save_loc)
    {
        return TrySaveModel(model, save_loc.string());
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

void osc::ActionSaveCurrentModelAs(const std::shared_ptr<IModelStatePair>& uim)
{
    App::upd().prompt_user_to_save_file_with_extension_async([uim](std::optional<std::filesystem::path> p)
    {
        if (not p) {
            return;  // user cancelled out of the prompt
        }

        if (not TrySaveModel(uim->getModel(), p->string())) {
            return;  // error saving the model file
        }

        const std::string oldPath = uim->getModel().getInputFileName();

        uim->updModel().setInputFileName(p->string());

        if (p != oldPath) {
            uim->commit("changed osim path");
        }
        uim->setUpToDateWithFilesystem(std::filesystem::last_write_time(*p));

        App::singleton<RecentFiles>()->push_back(*p);
    }, "osim");
}

void osc::ActionNewModel(Widget& parent)
{
    auto tab = std::make_unique<ModelEditorTab>(&parent);
    App::post_event<OpenTabEvent>(parent, std::move(tab));
}

void osc::ActionOpenModel(Widget& api)
{
    DoOpenFileViaDialog(api);
}

void osc::ActionOpenModel(Widget& api, const std::filesystem::path& path)
{
    OpenOsimInLoadingTab(api, path);
}

void osc::ActionSaveModelAsync(
    const std::shared_ptr<IModelStatePair>& model,
    std::function<void(bool)> callback)
{
    // Handling function that's passed to the dialog backend (if necessary)
    std::function<void(std::optional<std::filesystem::path>)> handle_file = [callback = std::move(callback), model](std::optional<std::filesystem::path> p)
    {
        if (not p) {
            callback(false);
            return;  // user probably cancelled out
        }

        if (not TrySaveModel(model->getModel(), *p)) {
            callback(false);  // there was an error saving the model
            return;
        }

        const std::string oldPath = model->getModel().getInputFileName();
        model->updModel().setInputFileName(p->string());

        if (p->string() != oldPath) {
            model->commit("changed osim path");
        }
        model->setUpToDateWithFilesystem(std::filesystem::last_write_time(*p));

        App::singleton<RecentFiles>()->push_back(*p);
        callback(true);
    };

    // Now figure out how to actually get/handle the path...
    if (const std::string& backing_path = model->getModel().getInputFileName();
        not backing_path.empty() and backing_path != "Unassigned") {

        // the model has an associated file
        //
        // we can save over this document - *IF* it's not an example file
        if (IsAnExampleFile(backing_path)) {
            App::upd().prompt_user_to_save_file_with_extension_async(std::move(handle_file), "osim");
        }
        else {
            handle_file(backing_path);
        }
    }
    else {
        // the model has no associated file, so prompt the user for a save
        // location
        App::upd().prompt_user_to_save_file_with_extension_async(std::move(handle_file), "osim");
    }
}

void osc::ActionTryDeleteSelectionFromEditedModel(IModelStatePair& uim)
{
    if (uim.isReadonly()) {
        return;
    }

    const OpenSim::Component* const selected = uim.getSelected();
    if (not selected) {
        return;
    }

    const OpenSim::ComponentPath selectedPath = GetAbsolutePath(*selected);

    const UID oldVersion = uim.getModelVersion();
    OpenSim::Model& mutModel = uim.updModel();
    OpenSim::Component* const mutComponent = FindComponentMut(mutModel, selectedPath);

    if (not mutComponent) {
        uim.setModelVersion(oldVersion);
        return;
    }

    const std::string selectedComponentName = mutComponent->getName();

    if (TryDeleteComponentFromModel(mutModel, *mutComponent)) {
        try {
            InitializeModel(mutModel);
            InitializeState(mutModel);

            std::stringstream ss;
            ss << "deleted " << selectedComponentName;
            uim.commit(std::move(ss).str());
        }
        catch (const std::exception&) {
            std::throw_with_nested(std::runtime_error{"error detected while deleting a component"});
        }
    }
    else {
        uim.setModelVersion(oldVersion);
    }
}

void osc::ActionDisableAllWrappingSurfaces(IModelStatePair& model)
{
    if (model.isReadonly()) {
        return;
    }

    try {
        OpenSim::Model& mutModel = model.updModel();
        DeactivateAllWrapObjectsIn(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.commit("disabled all wrapping surfaces");
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while disabling wrapping surfaces"});
    }
}

void osc::ActionEnableAllWrappingSurfaces(IModelStatePair& model)
{
    if (model.isReadonly()) {
        return;
    }

    try {
        OpenSim::Model& mutModel = model.updModel();
        ActivateAllWrapObjectsIn(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.commit("enabled all wrapping surfaces");
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while enabling wrapping surfaces"});
    }
}

bool osc::ActionLoadSTOFileAgainstModel(
    Widget& parent,
    const IModelStatePair& uim,
    const std::filesystem::path& stoPath)
{
    try {
        auto modelCopy = std::make_unique<OpenSim::Model>(uim.getModel());
        InitializeModel(*modelCopy);
        InitializeState(*modelCopy);

        auto simulation = std::make_shared<Simulation>(StoFileSimulation{std::move(modelCopy), stoPath, uim.getFixupScaleFactor(), uim.tryUpdEnvironment()});
        auto tab = std::make_unique<SimulationTab>(&parent, simulation);
        App::post_event<OpenTabEvent>(parent, std::move(tab));

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to load an STO file against the model"});
        return false;
    }
}

bool osc::ActionStartSimulatingModel(
    Widget& parent,
    const IModelStatePair& uim)
{
    BasicModelStatePair modelState{uim};
    ForwardDynamicSimulatorParams params = FromParamBlock(uim.tryUpdEnvironment()->getSimulationParams());

    auto simulation = std::make_shared<Simulation>(ForwardDynamicSimulation{std::move(modelState), params});
    auto tab = std::make_unique<SimulationTab>(&parent, std::move(simulation));
    App::post_event<OpenTabEvent>(parent, std::move(tab));

    return true;
}

bool osc::ActionUpdateModelFromBackingFile(UndoableModelStatePair& uim)
{
    const auto path = TryFindInputFile(uim.getModel());

    if (not path) {
        return false;  // there is no backing file
    }

    if (not std::filesystem::exists(*path)) {
        return false;  // the file does not exist? (e.g. because the user deleted it externally - #495)
    }

    const auto currentTimestamp = uim.getLastFilesystemWriteTime();
    const auto lastSaveTime = std::filesystem::last_write_time(*path);

    if (currentTimestamp >= lastSaveTime) {
        // the backing file is probably up-to-date with the in-memory representation
        //
        // (e.g. because OSC just saved it and set the timestamp appropriately)
        return false;
    }

    // else: there is a backing file and it's newer than what's in-memory, so reload
    try {
        log_info("file change detected: loading updated file");

        auto loadedModel = LoadModel(uim.getModel().getInputFileName());

        log_info("loaded updated file");

        uim.setModel(std::move(loadedModel));
        uim.commit("reloaded osim");
        uim.setUpToDateWithFilesystem(lastSaveTime);

        return true;
    }
    catch (const std::exception& ex) {
        log_error("error detected while trying to automatically load a model file: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionCopyModelPathToClipboard(const IModelStatePair& uim)
{
    auto path = TryFindInputFile(uim.getModel());

    if (not path) {
        return false;  // there is no backing file
    }

    set_clipboard_text(std::filesystem::weakly_canonical(*path).string());

    return true;
}

bool osc::ActionAutoscaleSceneScaleFactor(IModelStatePair& uim)
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

bool osc::ActionToggleFrames(IModelStatePair& uim)
{
    if (uim.isReadonly()) {
        return false;
    }

    try {
        OpenSim::Model& mutModel = uim.updModel();
        const bool newState = ToggleShowingFrames(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit(newState ? "shown frames" : "hidden frames");

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to toggle frames"});
        return false;
    }
}

bool osc::ActionToggleMarkers(IModelStatePair& uim)
{
    if (uim.isReadonly()) {
        return false;
    }

    try {
        OpenSim::Model& mutModel = uim.updModel();
        const bool newState = ToggleShowingMarkers(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit(newState ? "shown markers" : "hidden markers");

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to toggle markers"});
        return false;
    }
}

bool osc::ActionToggleContactGeometry(IModelStatePair& uim)
{
    if (uim.isReadonly()) {
        return false;
    }

    try {
        OpenSim::Model& mutModel = uim.updModel();
        const bool newState = ToggleShowingContactGeometry(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit(newState ? "shown contact geometry" : "hidden contact geometry");

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to toggle contact geometry"});
        return false;
    }
}

bool osc::ActionToggleForces(IModelStatePair& uim)
{
    if (uim.isReadonly()) {
        return false;
    }

    try {
        OpenSim::Model& mutModel = uim.updModel();
        const bool newState = ToggleShowingForces(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit(newState ? "shown forces" : "hidden forces");

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to toggle forces"});
        return false;
    }
}

bool osc::ActionToggleWrapGeometry(IModelStatePair& uim)
{
    if (uim.isReadonly()) {
        return false;
    }

    try {
        OpenSim::Model& mutModel = uim.updModel();
        const bool newState = ToggleShowingWrapGeometry(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit(newState ? "shown wrap geometry" : "hidden wrap geometry");

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to toggle wrap geometry"});
        return false;
    }
}

bool osc::ActionOpenOsimParentDirectory(const OpenSim::Model& model)
{
    if (not HasInputFileName(model)) {
        return false;
    }

    const std::filesystem::path p{model.getInputFileName()};
    open_file_in_os_default_application(p.parent_path());
    return true;
}

bool osc::ActionOpenOsimInExternalEditor(const OpenSim::Model& model)
{
    if (!HasInputFileName(model)) {
        return false;
    }

    open_file_in_os_default_application(model.getInputFileName());
    return true;
}

bool osc::ActionReloadOsimFromDisk(UndoableModelStatePair& uim, SceneCache& meshCache)
{
    const auto inputFile = TryFindInputFile(uim.getModel());

    if (not inputFile) {
        log_error("cannot reload the osim file: the model doesn't appear to have a backing file (is it saved?)");
        return false;
    }

    try {
        log_info("manual osim file reload requested: attempting to reload the file");
        auto p = LoadModel(*inputFile);
        log_info("loaded updated file");

        uim.setModel(std::move(p));
        uim.commit("reloaded from filesystem");
        uim.setUpToDateWithFilesystem(std::filesystem::last_write_time(*inputFile));

        // #594: purge the app-wide mesh cache so that any user edits to the underlying
        // mesh files are immediately visible after reloading
        //
        // this is useful for users that are actively editing the meshes of the model file
        meshCache.clear_meshes();

        return true;
    }
    catch (const std::exception& ex) {
        log_error("error detected while trying to reload a model file: %s", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::ActionAddOffsetFrameToPhysicalFrame(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& path)
{
    if (uim.isReadonly()) {
        return false;
    }

    const auto* const target = FindComponent<OpenSim::PhysicalFrame>(uim.getModel(), path);
    if (not target) {
        return false;
    }

    const std::string newPofName = target->getName() + "_offsetframe";

    auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pof->setName(newPofName);
    pof->setParentFrame(*target);

    const UID oldVersion = uim.getModelVersion();  // for rollbacks
    try {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutTarget = FindComponentMut<OpenSim::PhysicalFrame>(mutModel, path);
        if (not mutTarget) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a frame to " + path.toString()});
        return false;
    }
}

bool osc::CanRezeroJoint(IModelStatePair& uim, const OpenSim::ComponentPath& jointPath)
{
    if (uim.isReadonly()) {
        return false;
    }

    const auto* const joint = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (not joint) {
        return false;
    }

    // if the joint uses offset frames for both its parent and child frames then
    // it is possible to reorient those frames such that the joint's new zero
    // point is whatever the current arrangement is (effectively, by pre-transforming
    // the parent into the child and assuming a "zeroed" joint is an identity op)

    return dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&joint->getParentFrame()) != nullptr;
}

bool osc::ActionRezeroJoint(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& jointPath)
{
    if (uim.isReadonly()) {
        return false;
    }

    const auto* const target = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (not target) {
        return false;  // nothing/invalid component type specified
    }

    const auto* const parentPOF = dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&target->getParentFrame());
    if (not parentPOF) {
        return false;  // target has no parent frame
    }

    const OpenSim::ComponentPath parentPath = GetAbsolutePath(*parentPOF);
    const OpenSim::PhysicalFrame& childFrame = target->getChildFrame();
    const SimTK::Transform parentXform = parentPOF->getTransformInGround(uim.getState());
    const SimTK::Transform childXform = childFrame.getTransformInGround(uim.getState());
    const SimTK::Transform child2parent = parentXform.invert() * childXform;
    const SimTK::Transform newXform = parentPOF->getOffsetTransform() * child2parent;

    const UID oldVersion = uim.getModelVersion();  // for rollbacks
    try {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutJoint = FindComponentMut<OpenSim::Joint>(mutModel, jointPath);
        if (not mutJoint) {
            uim.setModelVersion(oldVersion);  // cannot find mutable version of the joint
            return false;
        }

        auto* const mutParent = FindComponentMut<OpenSim::PhysicalOffsetFrame>(mutModel, parentPath);
        if (not mutParent) {
            uim.setModelVersion(oldVersion);  // cannot find mutable version of the parent offset frame
            return false;
        }

        // else: perform model transformation

        const std::string jointName = mutJoint->getName();

        // first, zero all the joint's coordinates
        //
        // (we're assuming that the new transform performs the same function)
        for (int i = 0, nc = mutJoint->getProperty_coordinates().size(); i < nc; ++i) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to rezero a joint"});
        return false;
    }
}

bool osc::ActionAddParentOffsetFrameToJoint(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& jointPath)
{
    if (uim.isReadonly()) {
        return false;
    }

    const auto* const target = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (not target) {
        return false;
    }

    auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pf->setParentFrame(target->getParentFrame());

    const UID oldVersion = uim.getModelVersion();  // for rollbacks
    try {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a parent offset frame"});
        return false;
    }
}

bool osc::ActionAddChildOffsetFrameToJoint(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& jointPath)
{
    if (uim.isReadonly()) {
        return false;
    }

    const auto* const target = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (not target) {
        return false;
    }

    auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pf->setParentFrame(target->getChildFrame());

    const UID oldVersion = uim.getModelVersion();
    try {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutJoint = FindComponentMut<OpenSim::Joint>(mutModel, jointPath);
        if (not mutJoint) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a child offset frame"});
        return false;
    }
}

bool osc::ActionSetComponentName(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& path,
    const std::string& newName)
{
    if (uim.isReadonly()) {
        return false;
    }

    if (newName.empty()) {
        return false;
    }

    const OpenSim::Component* const target = FindComponent(uim.getModel(), path);
    if (not target) {
        return false;
    }

    const UID oldVersion = uim.getModelVersion();
    try {
        OpenSim::Model& mutModel = uim.updModel();

        OpenSim::Component* const mutComponent = FindComponentMut(mutModel, path);
        if (not mutComponent) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to set a component's name"});
        return false;
    }
}

bool osc::ActionChangeJointTypeTo(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& jointPath,
    std::unique_ptr<OpenSim::Joint> newType)
{
    if (uim.isReadonly()) {
        return false;
    }

    if (not newType) {
        log_error("new joint type provided to ChangeJointType function is nullptr: cannot continue: this is a developer error and should be reported");
        return false;
    }

    const auto* const target = FindComponent<OpenSim::Joint>(uim.getModel(), jointPath);
    if (not target) {
        log_error("could not find %s in the model", jointPath.toString().c_str());
        return false;
    }

    const auto* const owner = GetOwner<OpenSim::JointSet>(*target);
    if (not owner) {
        log_error("%s is not owned by an OpenSim::JointSet", jointPath.toString().c_str());
        return false;
    }

    const OpenSim::ComponentPath ownerPath = GetAbsolutePath(*owner);

    const std::optional<size_t> maybeIdx = FindJointInParentJointSet(*target);
    if (not maybeIdx) {
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
    try {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutParent = FindComponentMut<OpenSim::JointSet>(mutModel, ownerPath);
        if (not mutParent) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to change a joint's type"});
        return false;
    }
}

bool osc::ActionAttachGeometryToPhysicalFrame(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& physFramePath,
    std::unique_ptr<OpenSim::Geometry> geom)
{
    if (uim.isReadonly()) {
        return false;
    }

    const auto* const target = FindComponent<OpenSim::PhysicalFrame>(uim.getModel(), physFramePath);
    if (not target) {
        return false;
    }

    const UID oldVersion = uim.getModelVersion();
    try {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutPof = FindComponentMut<OpenSim::PhysicalFrame>(mutModel, physFramePath);
        if (not mutPof) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to attach geometry to the a physical frame"});
        return false;
    }
}

bool osc::ActionAssignContactGeometryToHCF(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& hcfPath,
    const OpenSim::ComponentPath& contactGeomPath)
{
    if (uim.isReadonly()) {
        return false;
    }

    const auto* const target = FindComponent<OpenSim::HuntCrossleyForce>(uim.getModel(), hcfPath);
    if (not target) {
        return false;
    }

    const auto* const geom = FindComponent<OpenSim::ContactGeometry>(uim.getModel(), contactGeomPath);
    if (not geom) {
        return false;
    }

    const UID oldVersion = uim.getModelVersion();
    try {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutHCF = FindComponentMut<OpenSim::HuntCrossleyForce>(mutModel, hcfPath);
        if (not mutHCF) {
            uim.setModelVersion(oldVersion);
            return false;
        }

        // calling this ensures at least one `OpenSim::HuntCrossleyForce::ContactParameters`
        // is present in the HCF
        mutHCF->getStaticFriction();
        OSC_ASSERT_ALWAYS(!empty(mutHCF->updContactParametersSet()));

        mutHCF->updContactParametersSet()[0].updGeometry().appendValue(geom->getName());
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit("added contact geometry");

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to assign contact geometry to a HCF"});
        return false;
    }
}

bool osc::ActionApplyPropertyEdit(IModelStatePair& uim, ObjectPropertyEdit& resp)
{
    if (uim.isReadonly()) {
        return false;
    }

    const UID oldVersion = uim.getModelVersion();
    try {
        OpenSim::Model& model = uim.updModel();

        OpenSim::Component* const component = FindComponentMut(model, resp.getComponentAbsPath());
        if (not component) {
            uim.setModelVersion(oldVersion);
            return false;
        }

        OpenSim::AbstractProperty* const prop = FindPropertyMut(*component, resp.getPropertyName());
        if (not prop) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to apply a property edit"});
        return false;
    }
}

bool osc::ActionAddPathPointToGeometryPath(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& geometryPathPath,
    const OpenSim::ComponentPath& pointPhysFrame)
{
    if (uim.isReadonly()) {
        return false;
    }

    const auto* const gp = FindComponent<OpenSim::GeometryPath>(uim.getModel(), geometryPathPath);
    if (not gp) {
        return false;
    }

    const auto* const pf = FindComponent<OpenSim::PhysicalFrame>(uim.getModel(), pointPhysFrame);
    if (not pf) {
        return false;
    }

    const size_t n = size(gp->getPathPointSet());
    const std::string name = gp->getName() + "-P" + std::to_string(n + 1);
    const SimTK::Vec3 position = {0.0f, 0.0f, 0.0f};

    const UID oldVersion = uim.getModelVersion();
    try {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutGP = FindComponentMut<OpenSim::GeometryPath>(mutModel, geometryPathPath);
        if (not mutGP) {
            uim.setModelVersion(oldVersion);
            return false;
        }

        const std::string gpName = mutGP->getName();

        mutGP->appendNewPathPoint(name, *pf, position);
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);

        // try to select the new path point, if possible, so that the user
        // can immediately see the grab handles etc. (#779)
        if (const auto* gpAfterFinalization = FindComponent<OpenSim::GeometryPath>(mutModel, geometryPathPath)) {
            const auto& pps = gpAfterFinalization->getPathPointSet();
            if (not empty(pps)) {
                uim.setSelected(&At(pps, ssize(pps) - 1));
            }
        }

        std::stringstream ss;
        ss << "added path point to " << gpName;
        uim.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a path point to a geometry path"});
        return false;
    }
}

bool osc::ActionAddPathPointToPathActuator(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& pathActuatorPath,
    const OpenSim::ComponentPath& pointPhysFrame)
{
    if (uim.isReadonly()) {
        return false;
    }

    const auto* const pa = FindComponent<OpenSim::PathActuator>(uim.getModel(), pathActuatorPath);
    if (not pa) {
        return false;
    }

    const auto* const pf = FindComponent<OpenSim::PhysicalFrame>(uim.getModel(), pointPhysFrame);
    if (not pf) {
        return false;
    }

    const size_t n = size(pa->getGeometryPath().getPathPointSet());
    const std::string name = pa->getName() + "-P" + std::to_string(n + 1);
    const SimTK::Vec3 position = {0.0f, 0.0f, 0.0f};

    const UID oldVersion = uim.getModelVersion();
    try {
        OpenSim::Model& mutModel = uim.updModel();

        auto* const mutPA = FindComponentMut<OpenSim::PathActuator>(mutModel, pathActuatorPath);
        if (not mutPA) {
            uim.setModelVersion(oldVersion);
            return false;
        }

        const std::string paName = mutPA->getName();

        mutPA->addNewPathPoint(name, *pf, position);
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);

        // try to select the new path point, if possible, so that the user
        // can immediately see the grab handles etc. (#779)
        if (const auto* paAfterFinalization = FindComponent<OpenSim::PathActuator>(mutModel, pathActuatorPath)) {
            const auto& pps = paAfterFinalization->getGeometryPath().getPathPointSet();
            if (not empty(pps)) {
                uim.setSelected(&At(pps, ssize(pps) -1));
            }
        }

        std::stringstream ss;
        ss << "added path point to " << paName;
        uim.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a path point to a path actuator"});
        return false;
    }
}

bool osc::ActionReassignComponentSocket(
    IModelStatePair& uim,
    const OpenSim::ComponentPath& componentAbsPath,
    const std::string& socketName,
    const OpenSim::Object& connectee,
    SocketReassignmentFlags flags,
    std::string& error)
{
    if (uim.isReadonly()) {
        return false;
    }

    // HOTFIX for #382
    //
    // OpenSim can segfault if certain types of circular joint connections to `/ground` are made.
    // This early-out error just ensures that OpenSim Creator isn't nuked by that OpenSim bug
    //
    // issue #3299 in opensim-core
    if (socketName == "child_frame" && &connectee == &uim.getModel().getGround()) {
        error = "Error: you cannot assign a joint's child frame to ground: this is a known bug in OpenSim (see issue #382 in ComputationalBiomechanicsLab/opensim-creator and issue #3299 in opensim-org/opensim-core)";
        return false;
    }

    const OpenSim::Component* const target = FindComponent(uim.getModel(), componentAbsPath);
    if (not target) {
        return false;
    }

    const UID oldVersion = uim.getModelVersion();

    OpenSim::Model& mutModel = uim.updModel();

    OpenSim::Component* const mutComponent = FindComponentMut(mutModel, componentAbsPath);
    if (not mutComponent) {
        uim.setModelVersion(oldVersion);
        return false;
    }

    OpenSim::AbstractSocket* const mutSocket = FindSocketMut(*mutComponent, socketName);
    if (not mutSocket) {
        uim.setModelVersion(oldVersion);
        return false;
    }

    try {
        const bool componentPropertiesReexpressed = flags & SocketReassignmentFlags::TryReexpressComponentInNewConnectee ?
            TryReexpressComponentSpatialPropertiesInNewConnectee(*mutComponent, connectee, uim.getState()) :
            false;

        if (componentPropertiesReexpressed) {
            FinalizeFromProperties(mutModel);
        }
        mutSocket->connect(connectee);
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        uim.commit("reassigned socket");

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to reassign a socket"});
        return false;
    }
}

osc::BodyDetails::BodyDetails() :
    centerOfMass{0.0f, 0.0f, 0.0f},
    inertia{1.0f, 1.0f, 1.0f},
    mass{1.0f},
    bodyName{"new_body"},
    jointTypeIndex{IndexOf<OpenSim::WeldJoint>(GetComponentRegistry<OpenSim::Joint>()).value_or(0)},
    maybeGeometry{nullptr},
    addOffsetFrames{true}
{}

bool osc::ActionAddBodyToModel(IModelStatePair& uim, const BodyDetails& details)
{
    if (uim.isReadonly()) {
        return false;
    }

    const auto* const parent = FindComponent<OpenSim::PhysicalFrame>(uim.getModel(), details.parentFrameAbsPath);
    if (not parent) {
        return false;
    }

    const auto com = to<SimTK::Vec3>(details.centerOfMass);
    const auto inertia = to<SimTK::Inertia>(details.inertia);
    const auto mass = static_cast<double>(details.mass);

    // create body
    auto body = std::make_unique<OpenSim::Body>(details.bodyName, mass, com, inertia);

    // create joint between body and whatever the frame is
    const OpenSim::Joint& jointProto = At(GetComponentRegistry<OpenSim::Joint>(), details.jointTypeIndex).prototype();
    std::unique_ptr<OpenSim::Joint> joint = MakeJoint(details, *body, jointProto, *parent);

    // attach decorative geom
    if (details.maybeGeometry) {
        AttachGeometry(*body, Clone(*details.maybeGeometry));
    }

    // mutate the model and perform the edit
    try {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a body to the model"});
        return false;
    }
}

bool osc::ActionAddComponentToModel(
    IModelStatePair& model,
    std::unique_ptr<OpenSim::Component> c)
{
    return ActionAddComponentToModel(model, std::move(c), OpenSim::ComponentPath{});
}

bool osc::ActionAddComponentToModel(
    IModelStatePair& model,
    std::unique_ptr<OpenSim::Component> c,
    const OpenSim::ComponentPath& desiredParent)
{
    if (model.isReadonly()) {
        return false;
    }

    if (c == nullptr) {
        return false;
    }

    try {
        OpenSim::Model& mutModel = model.updModel();

        const OpenSim::Component* added = nullptr;

        if (desiredParent.empty()) {
            added = &AddComponentToAppropriateSet(mutModel, std::move(c));
        }
        else if (OpenSim::Component* desired = FindComponentMut(mutModel, desiredParent)) {
            added = &AddComponent(*desired, std::move(c));
        }
        else {
            log_error("The target parent component, %s, could not be found: adding component to the model instead.", desiredParent.toString().c_str());
            added = &AddComponentToAppropriateSet(mutModel, std::move(c));
        }

        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.setSelected(added);

        std::stringstream ss;
        ss << "added " << added->getName();
        model.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a component to the model"});
        return false;
    }
}

bool osc::ActionAddWrapObjectToPhysicalFrame(
    IModelStatePair& model,
    const OpenSim::ComponentPath& physicalFramePath,
    std::unique_ptr<OpenSim::WrapObject> wrapObjPtr)
{
    if (model.isReadonly()) {
        return false;
    }

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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a wrap object to the model"});
        return false;
    }
}

bool osc::ActionAddWrapObjectToGeometryPathWraps(
    IModelStatePair& model,
    const OpenSim::GeometryPath& geomPath,
    const OpenSim::WrapObject& wrapObject)
{
    if (model.isReadonly()) {
        return false;
    }

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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a wrap object to a geometry path"});
        return false;
    }
}

bool osc::ActionRemoveWrapObjectFromGeometryPathWraps(
    IModelStatePair& model,
    const OpenSim::GeometryPath& geomPath,
    const OpenSim::WrapObject& wrapObject)
{
    if (model.isReadonly()) {
        return false;
    }

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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a wrap object to a geometry path"});
        return false;
    }
}

bool osc::ActionZeroAllCoordinates(IModelStatePair& model)
{
    if (model.isReadonly()) {
        return false;
    }

    try {
        OpenSim::Model& mutModel = model.updModel();
        for (auto& coordinate : mutModel.updComponentList<OpenSim::Coordinate>()) {
            const double rangeMin = min(coordinate.getRangeMin(), coordinate.getRangeMax());
            const double rangeMax = max(coordinate.getRangeMin(), coordinate.getRangeMax());
            coordinate.set_default_value(clamp(0.0, rangeMin, rangeMax));
        }
        InitializeModel(mutModel);
        InitializeState(mutModel);
        model.commit("zeroed all coordinates");
        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while zeroing all coordinates in the model"});
        return false;
    }
}

bool osc::ActionSetCoordinateSpeed(
    IModelStatePair& model,
    const OpenSim::Coordinate& coord,
    double newSpeed)
{
    if (model.isReadonly()) {
        return false;
    }

    const OpenSim::ComponentPath coordPath = GetAbsolutePath(coord);

    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutCoord = FindComponentMut<OpenSim::Coordinate>(mutModel, coordPath);
        if (not mutCoord) {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        // HACK: don't do a full model+state re-realization here: only do it
        //       when the caller wants to save the coordinate change
        mutCoord->setDefaultSpeedValue(newSpeed);
        mutCoord->setSpeedValue(mutModel.updWorkingState(), newSpeed);
        TryEquilibrateMusclesOrLogWarning(mutModel, mutModel.updWorkingState());
        mutModel.realizeDynamics(mutModel.updWorkingState());

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to set a coordinate's speed"});
        return false;
    }
}

bool osc::ActionSetCoordinateSpeedAndSave(
    IModelStatePair& model,
    const OpenSim::Coordinate& coord,
    double newSpeed)
{
    if (model.isReadonly()) {
        return false;
    }

    if (ActionSetCoordinateSpeed(model, coord, newSpeed)) {
        OpenSim::Model& mutModel = model.updModel();
        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "set " << coord.getName() << "'s speed";
        model.commit(std::move(ss).str());

        return true;
    }
    else {
        return false;  // the edit wasn't made
    }
}

bool osc::ActionSetCoordinateLockedAndSave(
    IModelStatePair& model,
    const OpenSim::Coordinate& coord, bool v)
{
    if (model.isReadonly()) {
        return false;
    }

    const OpenSim::ComponentPath coordPath = GetAbsolutePath(coord);

    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutCoord = FindComponentMut<OpenSim::Coordinate>(mutModel, coordPath);
        if (not mutCoord) {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        mutCoord->setDefaultLocked(v);
        mutCoord->setLocked(mutModel.updWorkingState(), v);
        TryEquilibrateMusclesOrLogWarning(mutModel, mutModel.updWorkingState());
        mutModel.realizeDynamics(mutModel.updWorkingState());

        std::stringstream ss;
        ss << (v ? "locked " : "unlocked ") << mutCoord->getName();
        model.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to lock a coordinate"});
        return false;
    }
}

// set the value of a coordinate, but don't save it to the model (yet)
bool osc::ActionSetCoordinateValue(
    IModelStatePair& model,
    const OpenSim::Coordinate& coord,
    double newValue)
{
    if (model.isReadonly()) {
        return false;
    }

    const OpenSim::ComponentPath coordPath = GetAbsolutePath(coord);

    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutCoord = FindComponentMut<OpenSim::Coordinate>(mutModel, coordPath);
        if (not mutCoord) {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        const double rangeMin = min(mutCoord->getRangeMin(), mutCoord->getRangeMax());
        const double rangeMax = max(mutCoord->getRangeMin(), mutCoord->getRangeMax());

        if (not (rangeMin <= newValue and newValue <= rangeMax)) {
            model.setModelVersion(oldVersion);  // the requested edit is outside the coordinate's allowed range
            return false;
        }

        // HACK: don't do a full model+state re-realization here: only do it
        //       when the caller wants to save the coordinate change
        mutCoord->setDefaultValue(newValue);
        mutCoord->setValue(mutModel.updWorkingState(), newValue);
        TryEquilibrateMusclesOrLogWarning(mutModel, mutModel.updWorkingState());
        mutModel.realizeDynamics(mutModel.updWorkingState());

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to set a coordinate's value"});
        return false;
    }
}

bool osc::ActionSetCoordinateValueAndSave(
    IModelStatePair& model,
    const OpenSim::Coordinate& coord,
    double newValue)
{
    if (model.isReadonly()) {
        return false;
    }

    if (ActionSetCoordinateValue(model, coord, newValue)) {
        OpenSim::Model& mutModel = model.updModel();

        // CAREFUL: ensure that *all* coordinate's default values are updated to reflect
        //          the current state
        //
        // You might be thinking "but, the caller only wanted to set one coordinate". You're
        // right, but OpenSim models can contain constraints where editing one coordinate causes
        // a bunch of other coordinates to change.
        //
        // See #345 for a longer explanation
        for (OpenSim::Coordinate& c : mutModel.updComponentList<OpenSim::Coordinate>()) {
            c.setDefaultValue(c.getValue(model.getState()));
        }

        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "set " << coord.getName() << " to " << ConvertCoordValueToDisplayValue(coord, newValue);
        model.commit(std::move(ss).str());

        return true;
    }
    else {
        return false;  // an edit wasn't made
    }
}

bool osc::ActionSetComponentAndAllChildrensIsVisibleTo(
    IModelStatePair& model,
    const OpenSim::ComponentPath& path,
    bool newVisibility)
{
    if (model.isReadonly()) {
        return false;
    }

    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        OpenSim::Component* const mutComponent = FindComponentMut(mutModel, path);
        if (not mutComponent) {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        TrySetAppearancePropertyIsVisibleTo(*mutComponent, newVisibility);

        for (OpenSim::Component& c : mutComponent->updComponentList()) {
            TrySetAppearancePropertyIsVisibleTo(c, newVisibility);
        }

        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "set " << path.getComponentName() << " visibility to " << newVisibility;
        model.commit(std::move(ss).str());

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to hide a component"});
        return false;
    }
}

bool osc::ActionShowOnlyComponentAndAllChildren(
    IModelStatePair& model,
    const OpenSim::ComponentPath& path)
{
    if (model.isReadonly()) {
        return false;
    }

    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        OpenSim::Component* const mutComponent = FindComponentMut(mutModel, path);
        if (not mutComponent) {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        // first, hide everything in the model
        for (OpenSim::Component& c : mutModel.updComponentList()) {
            TrySetAppearancePropertyIsVisibleTo(c, false);
        }

        // then show the intended component and its children
        TrySetAppearancePropertyIsVisibleTo(*mutComponent, true);
        for (OpenSim::Component& c : mutComponent->updComponentList()) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to hide a component"});
        return false;
    }
}

bool osc::ActionSetComponentAndAllChildrenWithGivenConcreteClassNameIsVisibleTo(
    IModelStatePair& model,
    const OpenSim::ComponentPath& root,
    std::string_view concreteClassName,
    bool newVisibility)
{
    if (model.isReadonly()) {
        return false;
    }

    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        OpenSim::Component* const mutComponent = FindComponentMut(mutModel, root);
        if (not mutComponent) {
            model.setModelVersion(oldVersion);  // can't find the coordinate within the provided model
            return false;
        }

        // first, hide everything in the model
        for (OpenSim::Component& c : mutModel.updComponentList()) {
            if (c.getConcreteClassName() == concreteClassName) {
                TrySetAppearancePropertyIsVisibleTo(c, newVisibility);
                for (OpenSim::Component& child : c.updComponentList()) {
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
            if (newVisibility) {
                ss << "showing ";
            }
            else {
                ss << "hiding ";
            }
            ss << concreteClassName;
            model.commit(std::move(ss).str());
        }

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to show/hide components of a given type"});
        return false;
    }
}

bool osc::ActionTranslateStation(
    IModelStatePair& model,
    const OpenSim::Station& station,
    const Vector3& deltaPosition)
{
    if (model.isReadonly()) {
        return false;
    }

    const OpenSim::ComponentPath stationPath = GetAbsolutePath(station);
    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutStation = FindComponentMut<OpenSim::Station>(mutModel, stationPath);
        if (not mutStation) {
            model.setModelVersion(oldVersion);  // the provided path isn't a station
            return false;
        }

        const SimTK::Vec3 originalPos = mutStation->get_location();
        const SimTK::Vec3 newPos = originalPos + to<SimTK::Vec3>(deltaPosition);

        // perform mutation
        mutStation->set_location(newPos);

        // HACK: don't perform a full model reinitialization because that would be very expensive
        // and it is very likely that it isn't necessary when dragging a station
        //
        // InitializeModel(mutModel);  // don't do this
        InitializeState(mutModel);

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to move a station"});
        return false;
    }
}

bool osc::ActionTranslateStationAndSave(
    IModelStatePair& model,
    const OpenSim::Station& station,
    const Vector3& deltaPosition)
{
    if (model.isReadonly()) {
        return false;
    }

    if (ActionTranslateStation(model, station, deltaPosition)) {
        OpenSim::Model& mutModel = model.updModel();
        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "translated " << station.getName();
        model.commit(std::move(ss).str());

        return true;
    }
    else {
        return false;  // edit wasn't made
    }
}

bool osc::ActionTranslatePathPoint(
    IModelStatePair& model,
    const OpenSim::PathPoint& pathPoint,
    const Vector3& deltaPosition)
{
    if (model.isReadonly()) {
        return false;
    }

    const OpenSim::ComponentPath ppPath = GetAbsolutePath(pathPoint);
    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutPathPoint = FindComponentMut<OpenSim::PathPoint>(mutModel, ppPath);
        if (not mutPathPoint) {
            model.setModelVersion(oldVersion);  // the provided path isn't a station
            return false;
        }

        const SimTK::Vec3 originalPos = mutPathPoint->get_location();
        const SimTK::Vec3 newPos = originalPos + to<SimTK::Vec3>(deltaPosition);

        // perform mutation
        mutPathPoint->setLocation(newPos);
        InitializeState(mutModel);

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to move a path point"});
        return false;
    }
}

bool osc::ActionTranslatePathPointAndSave(
    IModelStatePair& model,
    const OpenSim::PathPoint& pathPoint,
    const Vector3& deltaPosition)
{
    if (model.isReadonly()) {
        return false;
    }

    if (ActionTranslatePathPoint(model, pathPoint, deltaPosition)) {
        OpenSim::Model& mutModel = model.updModel();
        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "translated " << pathPoint.getName();
        model.commit(std::move(ss).str());

        return true;
    }
    else {
        return false;  // edit wasn't made
    }
}

bool osc::ActionTransformPofV2(
    IModelStatePair& model,
    const OpenSim::PhysicalOffsetFrame& pof,
    const Vector3& newTranslation,
    const EulerAngles& newEulers)
{
    if (model.isReadonly()) {
        return false;
    }

    const OpenSim::ComponentPath pofPath = GetAbsolutePath(pof);
    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutPof = FindComponentMut<OpenSim::PhysicalOffsetFrame>(mutModel, pofPath);
        if (not mutPof) {
            model.setModelVersion(oldVersion);  // the provided path isn't a station
            return false;
        }

        // perform mutation
        mutPof->set_translation(to<SimTK::Vec3>(newTranslation));
        mutPof->set_orientation(to<SimTK::Vec3>(newEulers));
        InitializeModel(mutModel);
        InitializeState(mutModel);

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to transform a POF"});
        return false;
    }
    return false;
}

bool osc::ActionTransformWrapObject(
    IModelStatePair& model,
    const OpenSim::WrapObject& wo,
    const Vector3& deltaPosition,
    const EulerAngles& newEulers)
{
    if (model.isReadonly()) {
        return false;
    }

    const OpenSim::ComponentPath pofPath = GetAbsolutePath(wo);
    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutPof = FindComponentMut<OpenSim::WrapObject>(mutModel, pofPath);
        if (not mutPof) {
            model.setModelVersion(oldVersion);  // the provided path isn't a station
            return false;
        }

        const SimTK::Vec3 originalPos = mutPof->get_translation();
        const SimTK::Vec3 newPos = originalPos + to<SimTK::Vec3>(deltaPosition);

        // perform mutation
        mutPof->set_translation(newPos);
        mutPof->set_xyz_body_rotation(to<SimTK::Vec3>(newEulers));
        InitializeModel(mutModel);
        InitializeState(mutModel);

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to transform a POF"});
        return false;
    }
    return false;
}

bool osc::ActionTransformContactGeometry(
    IModelStatePair& model,
    const OpenSim::ContactGeometry& contactGeom,
    const Vector3& deltaPosition,
    const EulerAngles& newEulers)
{
    if (model.isReadonly()) {
        return false;
    }

    const OpenSim::ComponentPath pofPath = GetAbsolutePath(contactGeom);
    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();

        auto* const mutGeom = FindComponentMut<OpenSim::ContactGeometry>(mutModel, pofPath);
        if (not mutGeom) {
            model.setModelVersion(oldVersion);  // the provided path doesn't exist in the model
            return false;
        }

        const SimTK::Vec3 originalPos = mutGeom->get_location();
        const SimTK::Vec3 newPos = originalPos + to<SimTK::Vec3>(deltaPosition);

        // perform mutation
        mutGeom->set_location(newPos);
        mutGeom->set_orientation(to<SimTK::Vec3>(newEulers));
        InitializeModel(mutModel);
        InitializeState(mutModel);

        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to transform a POF"});
        return false;
    }
    return false;
}

bool osc::ActionFitSphereToMesh(IModelStatePair& model, const OpenSim::Mesh& openSimMesh)
{
    if (model.isReadonly()) {
        return false;
    }

    // fit a sphere to the mesh
    Sphere sphere;
    try {
        const Mesh mesh = ToOscMeshBakeScaleFactors(model.getModel(), model.getState(), openSimMesh);
        sphere = FitSphere(mesh);
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to fit a sphere to a mesh"});
        return false;
    }

    // create an `OpenSim::OffsetFrame` expressed w.r.t. the same frame as the mesh that
    // places the origin-centered `OpenSim::Sphere` at the computed `origin`
    auto offsetFrame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    offsetFrame->setName("sphere_fit");
    offsetFrame->connectSocket_parent(dynamic_cast<const OpenSim::PhysicalFrame&>(openSimMesh.getFrame()));
    offsetFrame->setOffsetTransform(SimTK::Transform{to<SimTK::Vec3>(sphere.origin)});

    // create an origin-centered `OpenSim::Sphere` geometry to visually represent the computed
    // sphere
    auto openSimSphere = std::make_unique<OpenSim::Sphere>(sphere.radius);
    openSimSphere->setName("sphere_geom");
    openSimSphere->connectSocket_frame(*offsetFrame);
    UpdAppearanceToFittedGeom(openSimSphere->upd_Appearance());

    // perform undoable model mutation
    const OpenSim::ComponentPath openSimMeshPath = GetAbsolutePath(openSimMesh);
    const UID oldVersion = model.getModelVersion();
    try {
        OpenSim::Model& mutModel = model.updModel();
        auto* const mutOpenSimMesh = FindComponentMut<OpenSim::Mesh>(mutModel, openSimMeshPath);
        if (not mutOpenSimMesh) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a sphere fit to the OpenSim model"});
        return false;
    }

    return true;
}

bool osc::ActionFitEllipsoidToMesh(IModelStatePair& model, const OpenSim::Mesh& openSimMesh)
{
    if (model.isReadonly()) {
        return false;
    }

    // fit an ellipsoid to the mesh
    Ellipsoid ellipsoid;
    try {
        const Mesh mesh = ToOscMeshBakeScaleFactors(model.getModel(), model.getState(), openSimMesh);
        ellipsoid = FitEllipsoid(mesh);
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to fit an ellipsoid to a mesh"});
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
        m.col(0) = to<SimTK::Vec3>(directions[0]);
        m.col(1) = to<SimTK::Vec3>(directions[1]);
        m.col(2) = to<SimTK::Vec3>(directions[2]);
        const SimTK::Transform t{SimTK::Rotation{m}, to<SimTK::Vec3>(ellipsoid.origin)};
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
    try {
        OpenSim::Model& mutModel = model.updModel();
        auto* const mutOpenSimMesh = FindComponentMut<OpenSim::Mesh>(mutModel, openSimMeshPath);
        if (not mutOpenSimMesh) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a sphere fit to the OpenSim model"});
        return false;
    }

    return true;
}

bool osc::ActionFitPlaneToMesh(IModelStatePair& model, const OpenSim::Mesh& openSimMesh)
{
    if (model.isReadonly()) {
        return false;
    }

    // fit a plane to the mesh
    Plane plane;
    try {
        const Mesh mesh = ToOscMeshBakeScaleFactors(model.getModel(), model.getState(), openSimMesh);
        plane = FitPlane(mesh);
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to fit a plane to a mesh"});
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
        const Quaternion q = rotation({0.0f, 1.0f, 0.0f}, plane.normal);
        offsetFrame->setOffsetTransform(SimTK::Transform{to<SimTK::Rotation>(q), to<SimTK::Vec3>(plane.origin)});
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
    try {
        OpenSim::Model& mutModel = model.updModel();
        auto* const mutOpenSimMesh = FindComponentMut<OpenSim::Mesh>(mutModel, openSimMeshPath);
        if (not mutOpenSimMesh) {
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
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to add a sphere fit to the OpenSim model"});
        return false;
    }

    return true;
}

bool osc::ActionImportLandmarks(
    IModelStatePair& model,
    std::span<const lm::NamedLandmark> landmarks,
    std::optional<std::string> maybeName,
    [[maybe_unused]] std::optional<std::string> maybeTargetFrameAbsPath)
{
    if (model.isReadonly()) {
        return false;
    }

    try {
        OpenSim::Model& mutModel = model.updModel();

        OpenSim::PhysicalFrame* maybeTargetFrame = nullptr;
        if (maybeTargetFrameAbsPath) {
            auto* f = FindComponentMut<OpenSim::PhysicalFrame>(mutModel, *maybeTargetFrameAbsPath);
            if (f) {
                maybeTargetFrame = f;
            }
            else {
                std::stringstream msg;
                msg << "Could not find the specified frame in the model: " << *maybeTargetFrameAbsPath;
                throw std::runtime_error(std::move(msg).str());
            }
        }

        for (const auto& landmark : landmarks) {
            if (maybeTargetFrame) {
                // If the caller specified a target frame then the markers should be imported
                // as direct children of the target frame, rather than being dumped into the
                // generic markerset.
                AddComponent<OpenSim::Marker>(*maybeTargetFrame, landmark.name, *maybeTargetFrame, to<SimTK::Vec3>(landmark.position));
            } else {
                AddMarker(mutModel, landmark.name, mutModel.getGround(), to<SimTK::Vec3>(landmark.position));
            }
        }
        FinalizeConnections(mutModel);
        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "imported " << std::move(maybeName).value_or("markers");
        model.commit(std::move(ss).str());
        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to import landmarks to the model"});
        return false;
    }
}

void osc::ActionExportModelGraphToDotviz(const std::shared_ptr<IModelStatePair>& model)
{
    App::upd().prompt_user_to_save_file_with_extension_async([model](std::optional<std::filesystem::path> p)
    {
        if (not p) {
            return;  // user cancelled out of the prompt
        }

        if (std::ofstream of{*p}) {
            WriteComponentTopologyGraphAsDotViz(model->getModel(), of);
        }
        else {
            log_error("error opening %s for writing", p->string().c_str());
        }
    }, "dot");
}

bool osc::ActionExportModelGraphToDotvizClipboard(const OpenSim::Model& model)
{
    std::stringstream ss;
    WriteComponentTopologyGraphAsDotViz(model.getModel(), ss);
    set_clipboard_text(std::move(ss).str());
    return true;
}

bool osc::ActionExportModelMultibodySystemAsDotviz(const OpenSim::Model& model)
{
    std::stringstream ss;
    WriteModelMultibodySystemGraphAsDotViz(model.getModel(), ss);
    set_clipboard_text(std::move(ss).str());
    return true;
}

bool osc::ActionBakeStationDefinedFrames(IModelStatePair& model)
{
    if (model.isReadonly()) {
        return false;
    }

    // Ensure there is at least one `StationDefinedFrame`s in the model.
    {
        auto lst = model.getModel().getComponentList<OpenSim::StationDefinedFrame>();
        if (std::distance(lst.begin(), lst.end()) == 0) {
            return false;
        }
    }

    // Mutate the model by adding equivalent `PhysicalOffsetFrame`s to the
    // model, reattaching stuff to it, and then deleting the `StationDefinedFrame`.
    //
    // TODO:
    // - Create `PhysicalOffsetFrame` with a transform equivalent to the `StationDefinedFrame`
    // - Copy over anything that the `StationDefinedFrame` owns (e.g. component list, AttachedGeometry)
    // - Delete the `StationDefinedFrame` from the model.
    // - Add the `PhysicalOffsetFrame` into the model in the exact same location + name, so that
    //   all sockets, associations, etc. work as expected
    OpenSim::Model& mutModel = model.updModel();
    std::vector<OpenSim::StationDefinedFrame*> sdfsToDelete;
    std::vector<OpenSim::PhysicalOffsetFrame*> pofsToRename;
    for (auto& sdf : mutModel.updComponentList<OpenSim::StationDefinedFrame>()) {
        auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        // TODO: copy
        // - Subcomponents
        // - Wrap Obects
        pof->setName(sdf.getName() + "_tmp");
        const SimTK::Transform xform = sdf.findTransformInBaseFrame();
        pof->set_translation(xform.p());
        pof->set_orientation(xform.R().convertRotationToBodyFixedXYZ());
        pof->updProperty_attached_geometry().assign(sdf.getProperty_attached_geometry());
        pof->updProperty_WrapObjectSet().assign(sdf.getProperty_WrapObjectSet());
        pof->updSocket("parent").setConnecteePath(sdf.findBaseFrame().getAbsolutePathString());
        // Add it into the model
        auto& pofPtr = *pof;
        mutModel.updComponent(sdf.getAbsolutePath().getParentPath()).addComponent(pof.release());
        pofPtr.finalizeConnections(mutModel);
        // Reassign anything pointing to the SDF to instead point to the POF
        RecursivelyReassignAllSockets(mutModel,sdf, pofPtr);
        sdfsToDelete.push_back(&sdf);
        pofsToRename.push_back(&pofPtr);
    }
    for (size_t i = 0; i < sdfsToDelete.size(); ++i) {
        std::string name = sdfsToDelete[i]->getName();
        TryDeleteComponentFromModel(mutModel, *sdfsToDelete[i]);
        pofsToRename[i]->setName(name);
    }
    FinalizeConnections(mutModel);
    InitializeModel(mutModel);
    InitializeState(mutModel);
    model.commit("Bake `StationDefinedFrame`s");

    return true;
}

bool osc::ActionMoveMarkerToModelMarkerSet(IModelStatePair& model, const OpenSim::Marker& marker)
{
    if (model.isReadonly()) {
        return false;
    }

    const auto* owner = GetOwner(marker);
    if (not owner) {
        return false;  // The marker is either the root (uhh) or disowned
    }

    if (dynamic_cast<const OpenSim::MarkerSet*>(owner) and GetOwner<OpenSim::Model>(*owner) == &model.getModel()) {
        return false;  // The marker is already in the model's `MarkerSet`
    }

    // else: perform model mutation

    OpenSim::Model& mutModel = model.updModel();
    OpenSim::Component* mutOwner = UpdOwner(mutModel, marker);
    if (not mutOwner) {
        return false;  // Something went wrong trying to unlock/mutate the owner
    }
    auto* mutMarker = FindComponentMut<OpenSim::Marker>(mutModel, marker.getAbsolutePath());
    if (not mutMarker) {
        return false;  // Something went wrong trying to unlock/mutate the original `Marker`
    }
    std::unique_ptr<OpenSim::Marker> extracted = mutOwner->extractComponent(mutMarker);
    if (not extracted) {
        return false;  // Something went wrong extracting the marker from its current owner
    }
    const auto* extractedPtr = extracted.get();
    mutModel.addMarker(extracted.release());
    FinalizeConnections(mutModel);
    model.setSelected(extractedPtr);
    InitializeModel(mutModel);
    InitializeState(mutModel);
    std::stringstream msg;
    msg << "Moved " << extractedPtr->getName() << " to /markerset";
    model.commit(std::move(msg).str());

    return true;
}

bool osc::ActionTranslateContactHint(
    IModelStatePair& model,
    const OpenSim::Scholz2015GeometryPathObstacle& obstacle,
    const Vector3& deltaPosition)
{
    if (model.isReadonly()) {
        return false;
    }

    try {
        OpenSim::Model& mutModel = model.updModel();
        auto* mutObstacle = FindComponentMut<OpenSim::Scholz2015GeometryPathObstacle>(mutModel, obstacle.getAbsolutePath());
        if (not mutObstacle) {
            return false;  // Something went wrong trying to unlock/mutate the obstacle
        }
        mutObstacle->set_contact_hint(mutObstacle->get_contact_hint() + to<SimTK::Vec3>(deltaPosition));
        // Don't update state or reinitialize model: the wrapping requires a full rebuild, which happens
        // during `ActionSetContactHintAndSave`
        return true;
    }
    catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while moving a contact hint"});
        return false;
    }
}

bool osc::ActionTranslateContactHintAndSave(
    IModelStatePair& model,
    const OpenSim::Scholz2015GeometryPathObstacle& obstacle,
    const Vector3& deltaPosition)
{
    if (ActionTranslateContactHint(model, obstacle, deltaPosition)) {
        OpenSim::Model& mutModel = model.updModel();
        InitializeModel(mutModel);
        InitializeState(mutModel);

        std::stringstream ss;
        ss << "translated " << obstacle.getName();
        model.commit(std::move(ss).str());

        return true;
    }
    else {
        return false;  // edit wasn't made
    }
}

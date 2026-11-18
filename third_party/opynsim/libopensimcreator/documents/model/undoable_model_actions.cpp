#include "undoable_model_actions.h"

#include <libopensimcreator/documents/model/basic_model_state_pair_with_shared_environment.h>
#include <libopensimcreator/documents/model/environment.h>
#include <libopensimcreator/documents/model/undoable_model_state_pair.h>
#include <libopensimcreator/documents/simulation/forward_dynamic_simulation.h>
#include <libopensimcreator/documents/simulation/forward_dynamic_simulator_params.h>
#include <libopensimcreator/documents/simulation/simulation.h>
#include <libopensimcreator/documents/simulation/sto_file_simulation.h>
#include <libopensimcreator/platform/recent_files.h>
#include <libopensimcreator/ui/loading_tab.h>
#include <libopensimcreator/ui/model_editor/model_editor_tab.h>
#include <libopensimcreator/ui/shared/object_properties_editor.h>
#include <libopensimcreator/ui/simulation/simulation_tab.h>

#include <libopynsim/component_registry/component_registry.h>
#include <libopynsim/component_registry/static_component_registries.h>
#include <libopynsim/documents/file_filters.h>
#include <libopynsim/graphics/open_sim_decoration_generator.h>
#include <libopynsim/graphics/open_sim_decoration_options.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <libopynsim/utilities/shape_fitters.h>
#include <libopynsim/utilities/simbody_x_oscar.h>
#include <liboscar/graphics/scene/scene_cache.h>
#include <liboscar/maths/ellipsoid.h>
#include <liboscar/maths/ellipsoid_functions.h>
#include <liboscar/maths/math_helpers.h>
#include <liboscar/maths/plane.h>
#include <liboscar/maths/quaternion.h>
#include <liboscar/maths/sphere.h>
#include <liboscar/maths/vector.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/log.h>
#include <liboscar/platform/os.h>
#include <liboscar/ui/events/open_tab_event.h>
#include <liboscar/utilities/algorithms.h>
#include <liboscar/utilities/assertions.h>
#include <liboscar/utilities/filesystem_helpers.h>
#include <liboscar/utilities/uid.h>
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
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <typeinfo>
#include <utility>

using namespace osc;
using namespace opyn;

// helper functions
namespace
{
    void open_osim_in_loading_tab(Widget& parent, std::filesystem::path p)
    {
        auto tab = std::make_unique<LoadingTab>(&parent, std::move(p));
        App::post_event<OpenTabEvent>(parent, std::move(tab));
    }

    void do_open_file_via_dialog(Widget& api)
    {
        App::upd().prompt_user_to_select_file_async(
            [widget_ptr = api.weak_ref()](FileDialogResponse response) {
                if (not widget_ptr) {
                    return;  // widget was deleted at some point
                }

                if (response.has_error()) {
                    log_error("Error opening dialog: {}", response.error());
                    return;
                }

                for (const auto& path : response) {
                    open_osim_in_loading_tab(*widget_ptr, path);
                }
            },
            get_model_file_filters(),
            std::nullopt,  // initial directory
            true           // allow many
        );
    }

    bool is_an_example_file(const std::filesystem::path& path)
    {
        if (const auto example_models_directory_path =
                App::resource_filepath("OpenSimCreator/models")) {
            return is_subpath(*example_models_directory_path, path);
        } else {
            return false;
        }
    }

    bool try_save_model(const OpenSim::Model& model, const std::string& save_loc)
    {
        try {
            model.print(save_loc);
            log_info("saved model to {}", save_loc);
            return true;
        } catch (const OpenSim::Exception& ex) {
            log_error("error saving model: {}", ex.what());
            return false;
        }
    }

    bool try_save_model(const OpenSim::Model& model, const std::filesystem::path& save_loc)
    {
        return try_save_model(model, save_loc.string());
    }

    // create a "standard" OpenSim::Joint
    std::unique_ptr<OpenSim::Joint> make_joint(
        const BodyDetails& details,
        const OpenSim::Body& b,
        const OpenSim::Joint& joint_prototype,
        const OpenSim::PhysicalFrame& selected_pf)
    {
        std::unique_ptr<OpenSim::Joint> copy = clone(joint_prototype);
        copy->setName(details.joint_name);

        if (not details.add_offset_frames) {
            copy->connectSocket_parent_frame(selected_pf);
            copy->connectSocket_child_frame(b);
        } else {
            // add first offset frame as joint's parent
            {
                auto pof1 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                pof1->setParentFrame(selected_pf);
                pof1->setName(selected_pf.getName() + "_offset");

                // care: ownership change happens here (#642)
                OpenSim::PhysicalOffsetFrame& ref = add_frame(*copy, std::move(pof1));
                copy->connectSocket_parent_frame(ref);
            }

            // add second offset frame as joint's child
            {
                auto pof2 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                pof2->setParentFrame(b);
                pof2->setName(b.getName() + "_offset");

                // care: ownership change happens here (#642)
                OpenSim::PhysicalOffsetFrame& ref = add_frame(*copy, std::move(pof2));
                copy->connectSocket_child_frame(ref);
            }
        }

        return copy;
    }

    bool try_reexpress_component_spatial_properties_in_new_connectee(
        OpenSim::Component& component,
        const OpenSim::Object& new_connectee,
        const SimTK::State& state)
    {
        const auto* const new_frame = dynamic_cast<const OpenSim::Frame*>(&new_connectee);
        if (not new_frame) {
            return false;  // new connectee isn't a frame
        }

        const auto spatial_rep = try_get_spatial_representation(component, state);
        if (not spatial_rep) {
            return false;  // cannot represent the component spatially
        }

        const SimTK::Transform current_parent_to_ground = spatial_rep->parent_to_ground;
        const SimTK::Transform ground_to_new_connectee =
            new_frame->getTransformInGround(state).invert();
        const SimTK::Transform current_parent_to_new_connectee =
            ground_to_new_connectee * current_parent_to_ground;

        if (auto* positional_prop = find_simple_property_mut<SimTK::Vec3>(
                component, spatial_rep->location_vec3_property_name)) {
            const SimTK::Vec3 old_position = positional_prop->getValue();
            const SimTK::Vec3 new_position = current_parent_to_new_connectee * old_position;

            positional_prop->setValue(new_position);  // update property with new position
        }

        if (spatial_rep->maybe_orientation_vec3_eulers_property_name) {
            if (auto* orientational_prop = find_simple_property_mut<SimTK::Vec3>(
                    component, *spatial_rep->maybe_orientation_vec3_eulers_property_name)) {
                const SimTK::Rotation current_rotation_in_ground =
                    spatial_rep->parent_to_ground.R();
                const SimTK::Rotation ground_to_new_connectee_rotation =
                    new_frame->getRotationInGround(state).invert();
                const SimTK::Rotation current_parent_rotation_to_new_connectee_rotation =
                    ground_to_new_connectee_rotation * current_rotation_in_ground;

                const SimTK::Vec3 old_eulers = orientational_prop->getValue();
                SimTK::Rotation old_rotation;
                old_rotation.setRotationToBodyFixedXYZ(old_eulers);
                const SimTK::Rotation new_rotation =
                    current_parent_rotation_to_new_connectee_rotation * old_rotation;
                const SimTK::Vec3 new_eulers = new_rotation.convertRotationToBodyFixedXYZ();

                orientational_prop->setValue(new_eulers);
            }
        }

        return true;
    }

    // updates `appearance` to that of a fitted geometry
    void upd_appearance_to_fitted_geom(OpenSim::Appearance& appearance)
    {
        appearance.set_color({0.0, 1.0, 0.0});
        appearance.set_opacity(0.3);
    }
}

void osc::action_save_current_model_as(const std::shared_ptr<ModelStatePair>& uim)
{
    App::upd().prompt_user_to_save_file_with_extension_async(
        [uim](std::optional<std::filesystem::path> p) {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            if (not try_save_model(uim->get_model(), p->string())) {
                return;  // error saving the model file
            }

            const std::string old_path = uim->get_model().getInputFileName();

            uim->upd_model().setInputFileName(p->string());

            if (p != old_path) {
                uim->commit("changed osim path");
            }
            uim->set_up_to_date_with_filesystem(std::filesystem::last_write_time(*p));

            App::singleton<RecentFiles>()->push_back(*p);
        },
        "osim");
}

void osc::action_new_model(Widget& parent)
{
    auto tab = std::make_unique<ModelEditorTab>(&parent);
    App::post_event<OpenTabEvent>(parent, std::move(tab));
}

void osc::action_open_model(Widget& api)
{
    do_open_file_via_dialog(api);
}

void osc::action_open_model(Widget& api, const std::filesystem::path& path)
{
    open_osim_in_loading_tab(api, path);
}

void osc::action_save_model_async(
    const std::shared_ptr<ModelStatePair>& model, std::function<void(bool)> callback)
{
    // Handling function that's passed to the dialog backend (if necessary)
    std::function<void(std::optional<std::filesystem::path>)> handle_file =
        [callback = std::move(callback), model](std::optional<std::filesystem::path> p) {
            if (not p) {
                callback(false);
                return;  // user probably cancelled out
            }

            if (not try_save_model(model->get_model(), *p)) {
                callback(false);  // there was an error saving the model
                return;
            }

            const std::string old_path = model->get_model().getInputFileName();
            model->upd_model().setInputFileName(p->string());

            if (p->string() != old_path) {
                model->commit("changed osim path");
            }
            model->set_up_to_date_with_filesystem(std::filesystem::last_write_time(*p));

            App::singleton<RecentFiles>()->push_back(*p);
            callback(true);
        };

    // Now figure out how to actually get/handle the path...
    if (const std::string& backing_path = model->get_model().getInputFileName();
        not backing_path.empty() and backing_path != "Unassigned") {

        // the model has an associated file
        //
        // we can save over this document - *IF* it's not an example file
        if (is_an_example_file(backing_path)) {
            App::upd().prompt_user_to_save_file_with_extension_async(
                std::move(handle_file), "osim");
        } else {
            handle_file(backing_path);
        }
    } else {
        // the model has no associated file, so prompt the user for a save
        // location
        App::upd().prompt_user_to_save_file_with_extension_async(std::move(handle_file), "osim");
    }
}

void osc::action_try_delete_selection_from_edited_model(ModelStatePair& uim)
{
    if (uim.is_readonly()) {
        return;
    }

    const OpenSim::Component* const selected = uim.get_selected();
    if (not selected) {
        return;
    }

    const OpenSim::ComponentPath selected_path = get_absolute_path(*selected);

    const UID old_version = uim.get_model_version();
    OpenSim::Model& mut_model = uim.upd_model();
    OpenSim::Component* const mut_component = find_component_mut(mut_model, selected_path);

    if (not mut_component) {
        uim.set_model_version(old_version);
        return;
    }

    const std::string selected_component_name = mut_component->getName();

    if (try_delete_component_from_model(mut_model, *mut_component)) {
        try {
            initialize_model(mut_model);
            initialize_state(mut_model);

            std::stringstream ss;
            ss << "deleted " << selected_component_name;
            uim.commit(std::move(ss).str());
        } catch (const std::exception&) {
            std::throw_with_nested(std::runtime_error{"error detected while deleting a component"});
        }
    } else {
        uim.set_model_version(old_version);
    }
}

void osc::action_disable_all_wrapping_surfaces(ModelStatePair& model)
{
    if (model.is_readonly()) {
        return;
    }

    try {
        OpenSim::Model& mut_model = model.upd_model();
        deactivate_all_wrap_objects_in(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        model.commit("disabled all wrapping surfaces");
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while disabling wrapping surfaces"});
    }
}

void osc::action_enable_all_wrapping_surfaces(ModelStatePair& model)
{
    if (model.is_readonly()) {
        return;
    }

    try {
        OpenSim::Model& mut_model = model.upd_model();
        activate_all_wrap_objects_in(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        model.commit("enabled all wrapping surfaces");
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while enabling wrapping surfaces"});
    }
}

bool osc::action_load_sto_file_against_model(
    Widget& parent,
    const ModelStatePairWithSharedEnvironment& uim,
    const std::filesystem::path& sto_path)
{
    try {
        auto model_copy = std::make_unique<OpenSim::Model>(uim.get_model());
        initialize_model(*model_copy);
        initialize_state(*model_copy);

        auto simulation = std::make_shared<Simulation>(StoFileSimulation{
            std::move(model_copy),
            sto_path,
            uim.get_fixup_scale_factor(),
            uim.try_upd_environment()});
        auto tab = std::make_unique<SimulationTab>(&parent, simulation);
        App::post_event<OpenTabEvent>(parent, std::move(tab));

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{
                "error detected while trying to load an STO file against the model"});
        return false;
    }
}

bool osc::action_start_simulating_model(
    Widget& parent, const ModelStatePairWithSharedEnvironment& uim)
{
    BasicModelStatePairWithSharedEnvironment model_state{uim};
    ForwardDynamicSimulatorParams params =
        from_param_block(uim.try_upd_environment()->get_simulation_params());

    auto simulation =
        std::make_shared<Simulation>(ForwardDynamicSimulation{std::move(model_state), params});
    auto tab = std::make_unique<SimulationTab>(&parent, std::move(simulation));
    App::post_event<OpenTabEvent>(parent, std::move(tab));

    return true;
}

bool osc::action_update_model_from_backing_file(UndoableModelStatePair& uim)
{
    const auto path = try_find_input_file(uim.get_model());

    if (not path) {
        return false;  // there is no backing file
    }

    if (not std::filesystem::exists(*path)) {
        // the file does not exist? (e.g. because the user deleted it externally - #495).
        return false;
    }

    const auto current_timestamp = uim.get_last_filesystem_write_time();
    const auto last_save_time = std::filesystem::last_write_time(*path);

    if (current_timestamp >= last_save_time) {
        // the backing file is probably up-to-date with the in-memory representation
        //
        // (e.g. because OSC just saved it and set the timestamp appropriately)
        return false;
    }

    // else: there is a backing file and it's newer than what's in-memory, so reload
    try {
        log_info("file change detected: loading updated file");

        auto loaded_model = load_model(uim.get_model().getInputFileName());

        log_info("loaded updated file");

        uim.set_model(std::move(loaded_model));
        uim.commit("reloaded osim");
        uim.set_up_to_date_with_filesystem(last_save_time);

        return true;
    } catch (const std::exception& ex) {
        log_error("error detected while trying to automatically load a model file: {}", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::action_copy_model_path_to_clipboard(const ModelStatePair& uim)
{
    auto path = try_find_input_file(uim.get_model());

    if (not path) {
        return false;  // there is no backing file
    }

    set_clipboard_text(std::filesystem::weakly_canonical(*path).string());

    return true;
}

bool osc::action_autoscale_scene_scale_factor(ModelStatePair& uim)
{
    const float sf = get_recommended_scale_factor(
        *App::singleton<SceneCache>(App::resource_loader()),
        uim.get_model(),
        uim.get_state(),
        OpenSimDecorationOptions{});
    uim.set_fixup_scale_factor(sf);
    return true;
}

bool osc::action_toggle_frames(ModelStatePair& uim)
{
    if (uim.is_readonly()) {
        return false;
    }

    try {
        OpenSim::Model& mut_model = uim.upd_model();
        const bool new_state = toggle_showing_frames(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.commit(new_state ? "shown frames" : "hidden frames");

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to toggle frames"});
        return false;
    }
}

bool osc::action_toggle_markers(ModelStatePair& uim)
{
    if (uim.is_readonly()) {
        return false;
    }

    try {
        OpenSim::Model& mut_model = uim.upd_model();
        const bool new_state = toggle_showing_markers(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.commit(new_state ? "shown markers" : "hidden markers");

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to toggle markers"});
        return false;
    }
}

bool osc::action_toggle_contact_geometry(ModelStatePair& uim)
{
    if (uim.is_readonly()) {
        return false;
    }

    try {
        OpenSim::Model& mut_model = uim.upd_model();
        const bool new_state = toggle_showing_contact_geometry(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.commit(new_state ? "shown contact geometry" : "hidden contact geometry");

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to toggle contact geometry"});
        return false;
    }
}

bool osc::action_toggle_forces(ModelStatePair& uim)
{
    if (uim.is_readonly()) {
        return false;
    }

    try {
        OpenSim::Model& mut_model = uim.upd_model();
        const bool new_state = toggle_showing_forces(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.commit(new_state ? "shown forces" : "hidden forces");

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to toggle forces"});
        return false;
    }
}

bool osc::action_toggle_wrap_geometry(ModelStatePair& uim)
{
    if (uim.is_readonly()) {
        return false;
    }

    try {
        OpenSim::Model& mut_model = uim.upd_model();
        const bool new_state = toggle_showing_wrap_geometry(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.commit(new_state ? "shown wrap geometry" : "hidden wrap geometry");

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to toggle wrap geometry"});
        return false;
    }
}

bool osc::action_open_osim_parent_directory(const OpenSim::Model& model)
{
    if (not has_input_file_name(model)) {
        return false;
    }

    const std::filesystem::path p{model.getInputFileName()};
    open_file_in_os_default_application(p.parent_path());
    return true;
}

bool osc::action_open_osim_in_external_editor(const OpenSim::Model& model)
{
    if (!has_input_file_name(model)) {
        return false;
    }

    open_file_in_os_default_application(model.getInputFileName());
    return true;
}

bool osc::action_reload_osim_from_disk(UndoableModelStatePair& uim, SceneCache& mesh_cache)
{
    const auto input_file = try_find_input_file(uim.get_model());

    if (not input_file) {
        log_error(
            "cannot reload the osim file: the model doesn't appear to have a backing file (is it "
            "saved?)");
        return false;
    }

    try {
        log_info("manual osim file reload requested: attempting to reload the file");
        auto p = load_model(*input_file);
        log_info("loaded updated file");

        uim.set_model(std::move(p));
        uim.commit("reloaded from filesystem");
        uim.set_up_to_date_with_filesystem(std::filesystem::last_write_time(*input_file));

        // #594: purge the app-wide mesh cache so that any user edits to the underlying
        // mesh files are immediately visible after reloading
        //
        // this is useful for users that are actively editing the meshes of the model file
        mesh_cache.clear_meshes();

        return true;
    } catch (const std::exception& ex) {
        log_error("error detected while trying to reload a model file: {}", ex.what());
        uim.rollback();
        return false;
    }
}

bool osc::action_add_offset_frame_to_physical_frame(
    ModelStatePair& uim, const OpenSim::ComponentPath& path)
{
    if (uim.is_readonly()) {
        return false;
    }

    const auto* const target = find_component<OpenSim::PhysicalFrame>(uim.get_model(), path);
    if (not target) {
        return false;
    }

    const std::string new_pof_name = target->getName() + "_offsetframe";

    auto pof = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pof->setName(new_pof_name);
    pof->setParentFrame(*target);

    const UID old_version = uim.get_model_version();  // for rollbacks
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        auto* const mut_target = find_component_mut<OpenSim::PhysicalFrame>(mut_model, path);
        if (not mut_target) {
            uim.set_model_version(old_version);
            return false;
        }

        OpenSim::PhysicalOffsetFrame& pof_ref = add_component(*mut_target, std::move(pof));
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.set_selected(&pof_ref);
        uim.commit("added " + new_pof_name);

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to add a frame to " + path.toString()});
        return false;
    }
}

bool osc::can_rezero_joint(ModelStatePair& uim, const OpenSim::ComponentPath& joint_path)
{
    if (uim.is_readonly()) {
        return false;
    }

    const auto* const joint = find_component<OpenSim::Joint>(uim.get_model(), joint_path);
    if (not joint) {
        return false;
    }

    // if the joint uses offset frames for both its parent and child frames then
    // it is possible to reorient those frames such that the joint's new zero
    // point is whatever the current arrangement is (effectively, by pre-transforming
    // the parent into the child and assuming a "zeroed" joint is an identity op)

    return dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&joint->getParentFrame()) != nullptr;
}

bool osc::action_rezero_joint(ModelStatePair& uim, const OpenSim::ComponentPath& joint_path)
{
    if (uim.is_readonly()) {
        return false;
    }

    const auto* const target = find_component<OpenSim::Joint>(uim.get_model(), joint_path);
    if (not target) {
        return false;  // nothing/invalid component type specified
    }

    const auto* const parent_pof =
        dynamic_cast<const OpenSim::PhysicalOffsetFrame*>(&target->getParentFrame());
    if (not parent_pof) {
        return false;  // target has no parent frame
    }

    const OpenSim::ComponentPath parent_path = get_absolute_path(*parent_pof);
    const OpenSim::PhysicalFrame& child_frame = target->getChildFrame();
    const SimTK::Transform parent_xform = parent_pof->getTransformInGround(uim.get_state());
    const SimTK::Transform child_xform = child_frame.getTransformInGround(uim.get_state());
    const SimTK::Transform child2parent = parent_xform.invert() * child_xform;
    const SimTK::Transform new_xform = parent_pof->getOffsetTransform() * child2parent;

    const UID old_version = uim.get_model_version();  // for rollbacks
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        auto* const mut_joint = find_component_mut<OpenSim::Joint>(mut_model, joint_path);
        if (not mut_joint) {
            uim.set_model_version(old_version);  // cannot find mutable version of the joint
            return false;
        }

        auto* const mut_parent =
            find_component_mut<OpenSim::PhysicalOffsetFrame>(mut_model, parent_path);
        if (not mut_parent) {
            // cannot find mutable version of the parent offset frame
            uim.set_model_version(old_version);
            return false;
        }

        // else: perform model transformation

        const std::string joint_name = mut_joint->getName();

        // first, zero all the joint's coordinates
        //
        // (we're assuming that the new transform performs the same function)
        for (int i = 0, nc = mut_joint->getProperty_coordinates().size(); i < nc; ++i) {
            mut_joint->upd_coordinates(i).setDefaultValue(0.0);
        }

        // then set the parent offset frame's transform to "do the work"
        mut_parent->setOffsetTransform(new_xform);

        // and then put the model back into a valid state, ready for committing etc.
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.commit("rezeroed " + joint_name);

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to rezero a joint"});
        return false;
    }
}

bool osc::action_add_parent_offset_frame_to_joint(
    ModelStatePair& uim, const OpenSim::ComponentPath& joint_path)
{
    if (uim.is_readonly()) {
        return false;
    }

    const auto* const target = find_component<OpenSim::Joint>(uim.get_model(), joint_path);
    if (not target) {
        return false;
    }

    auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pf->setParentFrame(target->getParentFrame());

    const UID old_version = uim.get_model_version();  // for rollbacks
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        auto* const mut_joint = find_component_mut<OpenSim::Joint>(mut_model, joint_path);
        if (not mut_joint) {
            uim.set_model_version(old_version);
            return false;
        }

        const std::string joint_name = mut_joint->getName();

        mut_joint->connectSocket_parent_frame(*pf);
        add_frame(*mut_joint, std::move(pf));
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.commit("added " + joint_name);

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to add a parent offset frame"});
        return false;
    }
}

bool osc::action_add_child_offset_frame_to_joint(
    ModelStatePair& uim, const OpenSim::ComponentPath& joint_path)
{
    if (uim.is_readonly()) {
        return false;
    }

    const auto* const target = find_component<OpenSim::Joint>(uim.get_model(), joint_path);
    if (not target) {
        return false;
    }

    auto pf = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    pf->setParentFrame(target->getChildFrame());

    const UID old_version = uim.get_model_version();
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        auto* const mut_joint = find_component_mut<OpenSim::Joint>(mut_model, joint_path);
        if (not mut_joint) {
            uim.set_model_version(old_version);
            return false;
        }

        const std::string joint_name = mut_joint->getName();

        mut_joint->connectSocket_child_frame(*pf);
        add_frame(*mut_joint, std::move(pf));
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.commit("added " + joint_name);

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to add a child offset frame"});
        return false;
    }
}

bool osc::action_set_component_name(
    ModelStatePair& uim, const OpenSim::ComponentPath& path, const std::string& new_name)
{
    if (uim.is_readonly()) {
        return false;
    }

    if (new_name.empty()) {
        return false;
    }

    const OpenSim::Component* const target = find_component(uim.get_model(), path);
    if (not target) {
        return false;
    }

    const UID old_version = uim.get_model_version();
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        OpenSim::Component* const mut_component = find_component_mut(mut_model, path);
        if (not mut_component) {
            uim.set_model_version(old_version);
            return false;
        }

        const std::string old_name = mut_component->getName();
        mut_component->setName(new_name);
        finalize_connections(mut_model);  // because pointers need to know the new name
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.set_selected(mut_component);  // because the name changed

        std::stringstream ss;
        ss << "renamed " << old_name << " to " << new_name;
        uim.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to set a component's name"});
        return false;
    }
}

bool osc::action_change_joint_type_to(
    ModelStatePair& uim,
    const OpenSim::ComponentPath& joint_path,
    std::unique_ptr<OpenSim::Joint> new_type)
{
    if (uim.is_readonly()) {
        return false;
    }

    if (not new_type) {
        log_error(
            "new joint type provided to ChangeJointType function is nullptr: cannot continue: this "
            "is a developer error and should be reported");
        return false;
    }

    const auto* const target = find_component<OpenSim::Joint>(uim.get_model(), joint_path);
    if (not target) {
        log_error("could not find {} in the model", joint_path.toString());
        return false;
    }

    const auto* const owner = get_owner<OpenSim::JointSet>(*target);
    if (not owner) {
        log_error("{} is not owned by an OpenSim::JointSet", joint_path.toString());
        return false;
    }

    const OpenSim::ComponentPath owner_path = get_absolute_path(*owner);

    const std::optional<size_t> maybe_idx = find_joint_in_parent_joint_set(*target);
    if (not maybe_idx) {
        log_error("{} could not be found in its owner", joint_path.toString());
        return false;
    }

    const size_t idx = *maybe_idx;

    const std::string old_type_name = target->getConcreteClassName();
    const std::string new_type_name = new_type->getConcreteClassName();

    copy_common_joint_properties(*target, *new_type);

    // perform model update by overwriting the old joint in model
    //
    // note: this will invalidate the input joint, because the
    // OpenSim::JointSet container will automatically kill it

    const UID old_version = uim.get_model_version();
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        auto* const mut_parent = find_component_mut<OpenSim::JointSet>(mut_model, owner_path);
        if (not mut_parent) {
            uim.set_model_version(old_version);
            return false;
        }

        const OpenSim::Joint& joint_ref = assign(*mut_parent, idx, std::move(new_type));
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.set_selected(&joint_ref);

        std::stringstream ss;
        ss << "changed " << old_type_name << " to " << new_type_name;
        uim.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to change a joint's type"});
        return false;
    }
}

bool osc::action_attach_geometry_to_physical_frame(
    ModelStatePair& uim,
    const OpenSim::ComponentPath& phys_frame_path,
    std::unique_ptr<OpenSim::Geometry> geom)
{
    if (uim.is_readonly()) {
        return false;
    }

    const auto* const target =
        find_component<OpenSim::PhysicalFrame>(uim.get_model(), phys_frame_path);
    if (not target) {
        return false;
    }

    const UID old_version = uim.get_model_version();
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        auto* const mut_pof =
            find_component_mut<OpenSim::PhysicalFrame>(mut_model, phys_frame_path);
        if (not mut_pof) {
            uim.set_model_version(old_version);
            return false;
        }

        const std::string pof_name = mut_pof->getName();

        attach_geometry(*mut_pof, std::move(geom));
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);

        std::stringstream ss;
        ss << "attached geometry to " << pof_name;
        uim.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{
                "error detected while trying to attach geometry to the a physical frame"});
        return false;
    }
}

bool osc::action_assign_contact_geometry_to_hcf(
    ModelStatePair& uim,
    const OpenSim::ComponentPath& hcf_path,
    const OpenSim::ComponentPath& contact_geom_path)
{
    if (uim.is_readonly()) {
        return false;
    }

    const auto* const target =
        find_component<OpenSim::HuntCrossleyForce>(uim.get_model(), hcf_path);
    if (not target) {
        return false;
    }

    const auto* const geom =
        find_component<OpenSim::ContactGeometry>(uim.get_model(), contact_geom_path);
    if (not geom) {
        return false;
    }

    const UID old_version = uim.get_model_version();
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        auto* const mut_hcf = find_component_mut<OpenSim::HuntCrossleyForce>(mut_model, hcf_path);
        if (not mut_hcf) {
            uim.set_model_version(old_version);
            return false;
        }

        // calling this ensures at least one `OpenSim::HuntCrossleyForce::ContactParameters`
        // is present in the HCF
        mut_hcf->getStaticFriction();
        OSC_ASSERT_ALWAYS(!empty(mut_hcf->updContactParametersSet()));

        mut_hcf->updContactParametersSet()[0].updGeometry().appendValue(geom->getName());
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.commit("added contact geometry");

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to assign contact geometry to a HCF"});
        return false;
    }
}

bool osc::action_apply_property_edit(ModelStatePair& uim, ObjectPropertyEdit& resp)
{
    if (uim.is_readonly()) {
        return false;
    }

    const UID old_version = uim.get_model_version();
    try {
        OpenSim::Model& model = uim.upd_model();

        OpenSim::Component* const component =
            find_component_mut(model, resp.get_component_abs_path());
        if (not component) {
            uim.set_model_version(old_version);
            return false;
        }

        OpenSim::AbstractProperty* const prop =
            find_property_mut(*component, resp.get_property_name());
        if (not prop) {
            uim.set_model_version(old_version);
            return false;
        }

        const std::string prop_name = prop->getName();

        resp.apply(*prop);

        const std::string new_value = prop->toStringForDisplay(3);

        initialize_model(model);
        initialize_state(model);

        std::stringstream ss;
        ss << "set " << prop_name << " to " << new_value;
        uim.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to apply a property edit"});
        return false;
    }
}

bool osc::action_add_path_point_to_geometry_path(
    ModelStatePair& uim,
    const OpenSim::ComponentPath& geometry_path_path,
    const OpenSim::ComponentPath& point_phys_frame)
{
    if (uim.is_readonly()) {
        return false;
    }

    const auto* const gp =
        find_component<OpenSim::GeometryPath>(uim.get_model(), geometry_path_path);
    if (not gp) {
        return false;
    }

    const auto* const pf =
        find_component<OpenSim::PhysicalFrame>(uim.get_model(), point_phys_frame);
    if (not pf) {
        return false;
    }

    const size_t n = size(gp->getPathPointSet());
    const std::string name = gp->getName() + "-P" + std::to_string(n + 1);
    const SimTK::Vec3 position = {0.0f, 0.0f, 0.0f};

    const UID old_version = uim.get_model_version();
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        auto* const mut_gp =
            find_component_mut<OpenSim::GeometryPath>(mut_model, geometry_path_path);
        if (not mut_gp) {
            uim.set_model_version(old_version);
            return false;
        }

        const std::string gp_name = mut_gp->getName();

        mut_gp->appendNewPathPoint(name, *pf, position);
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);

        // try to select the new path point, if possible, so that the user
        // can immediately see the grab handles etc. (#779)
        if (const auto* gp_after_finalization =
                find_component<OpenSim::GeometryPath>(mut_model, geometry_path_path)) {
            const auto& pps = gp_after_finalization->getPathPointSet();
            if (not empty(pps)) {
                uim.set_selected(&opyn::at(pps, ssize(pps) - 1));
            }
        }

        std::stringstream ss;
        ss << "added path point to " << gp_name;
        uim.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{
                "error detected while trying to add a path point to a geometry path"});
        return false;
    }
}

bool osc::action_add_path_point_to_path_actuator(
    ModelStatePair& uim,
    const OpenSim::ComponentPath& path_actuator_path,
    const OpenSim::ComponentPath& point_phys_frame)
{
    if (uim.is_readonly()) {
        return false;
    }

    const auto* const pa =
        find_component<OpenSim::PathActuator>(uim.get_model(), path_actuator_path);
    if (not pa) {
        return false;
    }

    const auto* const pf =
        find_component<OpenSim::PhysicalFrame>(uim.get_model(), point_phys_frame);
    if (not pf) {
        return false;
    }

    const auto* const gp = dynamic_cast<const OpenSim::GeometryPath*>(&pa->getPath());
    if (not gp) {
        log_warn(
            "Cannot add a path point to an AbstractGeometryPath that isn't a normal GeometryPath");
        return false;
    }

    const size_t n = size(gp->getPathPointSet());
    const std::string name = pa->getName() + "-P" + std::to_string(n + 1);
    const SimTK::Vec3 position = {0.0f, 0.0f, 0.0f};

    const UID old_version = uim.get_model_version();
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        auto* const mut_pa =
            find_component_mut<OpenSim::PathActuator>(mut_model, path_actuator_path);
        if (not mut_pa) {
            uim.set_model_version(old_version);
            return false;
        }

        const std::string pa_name = mut_pa->getName();

        mut_pa->addNewPathPoint(name, *pf, position);
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);

        // try to select the new path point, if possible, so that the user
        // can immediately see the grab handles etc. (#779)
        if (const auto* pa_after_finalization =
                find_component<OpenSim::PathActuator>(mut_model, path_actuator_path)) {
            if (const auto* gp_after_finalization =
                    dynamic_cast<const OpenSim::GeometryPath*>(&pa_after_finalization->getPath())) {

                const auto& pps = gp_after_finalization->getPathPointSet();
                if (not empty(pps)) {
                    uim.set_selected(&opyn::at(pps, ssize(pps) - 1));
                }
            }
        }

        std::stringstream ss;
        ss << "added path point to " << pa_name;
        uim.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{
                "error detected while trying to add a path point to a path actuator"});
        return false;
    }
}

bool osc::action_reassign_component_socket(
    ModelStatePair& uim,
    const OpenSim::ComponentPath& component_abs_path,
    const std::string& socket_name,
    const OpenSim::Object& connectee,
    SocketReassignmentFlags flags,
    std::string& error)
{
    if (uim.is_readonly()) {
        return false;
    }

    // HOTFIX for #382
    //
    // OpenSim can segfault if certain types of circular joint connections to `/ground` are made.
    // This early-out error just ensures that OpenSim Creator isn't nuked by that OpenSim bug
    //
    // issue #3299 in opensim-core
    if (socket_name == "child_frame" && &connectee == &uim.get_model().getGround()) {
        error = "Error: you cannot assign a joint's child frame to ground: this is a known bug in "
                "OpenSim (see issue #382 in ComputationalBiomechanicsLab/opensim-creator and issue "
                "#3299 in opensim-org/opensim-core)";
        return false;
    }

    const OpenSim::Component* const target = find_component(uim.get_model(), component_abs_path);
    if (not target) {
        return false;
    }

    const UID old_version = uim.get_model_version();

    OpenSim::Model& mut_model = uim.upd_model();

    OpenSim::Component* const mut_component = find_component_mut(mut_model, component_abs_path);
    if (not mut_component) {
        uim.set_model_version(old_version);
        return false;
    }

    OpenSim::AbstractSocket* const mut_socket = find_socket_mut(*mut_component, socket_name);
    if (not mut_socket) {
        uim.set_model_version(old_version);
        return false;
    }

    try {
        const bool component_properties_reexpressed =
            flags & SocketReassignmentFlags::TryReexpressComponentInNewConnectee
                ? try_reexpress_component_spatial_properties_in_new_connectee(
                      *mut_component, connectee, uim.get_state())
                : false;

        if (component_properties_reexpressed) {
            finalize_from_properties(mut_model);
        }
        mut_socket->connect(connectee);
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.commit("reassigned socket");

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to reassign a socket"});
        return false;
    }
}

osc::BodyDetails::BodyDetails() :
    joint_type_index{
        index_of<OpenSim::WeldJoint>(get_component_registry<OpenSim::Joint>()).value_or(0)}
{}

bool osc::action_add_body_to_model(ModelStatePair& uim, const BodyDetails& details)
{
    if (uim.is_readonly()) {
        return false;
    }

    const auto* const parent =
        find_component<OpenSim::PhysicalFrame>(uim.get_model(), details.parent_frame_abs_path);
    if (not parent) {
        return false;
    }

    const auto com = to<SimTK::Vec3>(details.center_of_mass);
    const auto inertia = to<SimTK::Inertia>(details.inertia);
    const auto mass = static_cast<double>(details.mass);

    // create body
    auto body = std::make_unique<OpenSim::Body>(details.body_name, mass, com, inertia);

    // create joint between body and whatever the frame is
    const OpenSim::Joint& joint_proto =
        get_component_registry<OpenSim::Joint>().at(details.joint_type_index).prototype();
    std::unique_ptr<OpenSim::Joint> joint = make_joint(details, *body, joint_proto, *parent);

    // attach decorative geom
    if (details.maybe_geometry) {
        attach_geometry(*body, clone(*details.maybe_geometry));
    }

    // mutate the model and perform the edit
    try {
        OpenSim::Model& mut_model = uim.upd_model();

        add_joint(mut_model, std::move(joint));
        OpenSim::Body& body_ref = add_body(mut_model, std::move(body));
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        uim.set_selected(&body_ref);

        std::stringstream ss;
        ss << "added " << body_ref.getName();
        uim.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to add a body to the model"});
        return false;
    }
}

bool osc::action_add_component_to_model(
    ModelStatePair& model, std::unique_ptr<OpenSim::Component> c)
{
    return action_add_component_to_model(model, std::move(c), OpenSim::ComponentPath{});
}

bool osc::action_add_component_to_model(
    ModelStatePair& model,
    std::unique_ptr<OpenSim::Component> c,
    const OpenSim::ComponentPath& desired_parent)
{
    if (model.is_readonly()) {
        return false;
    }

    if (c == nullptr) {
        return false;
    }

    try {
        OpenSim::Model& mut_model = model.upd_model();

        const OpenSim::Component* added = nullptr;

        if (desired_parent.empty()) {
            added = &add_component_to_appropriate_set(mut_model, std::move(c));
        } else if (OpenSim::Component* desired = find_component_mut(mut_model, desired_parent)) {
            added = &add_component(*desired, std::move(c));
        } else {
            log_error(
                "The target parent component, {}, could not be found: adding component to the "
                "model instead.",
                desired_parent.toString());
            added = &add_component_to_appropriate_set(mut_model, std::move(c));
        }

        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        model.set_selected(added);

        std::stringstream ss;
        ss << "added " << added->getName();
        model.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to add a component to the model"});
        return false;
    }
}

bool osc::action_add_wrap_object_to_physical_frame(
    ModelStatePair& model,
    const OpenSim::ComponentPath& physical_frame_path,
    std::unique_ptr<OpenSim::WrapObject> wrap_obj_ptr)
{
    if (model.is_readonly()) {
        return false;
    }

    OSC_ASSERT(wrap_obj_ptr != nullptr);

    if (!find_component<OpenSim::PhysicalFrame>(model.get_model(), physical_frame_path)) {
        return false;  // cannot find the `OpenSim::PhysicalFrame` in the model
    }

    try {
        OpenSim::Model& mut_model = model.upd_model();
        auto* frame = find_component_mut<OpenSim::PhysicalFrame>(mut_model, physical_frame_path);
        OSC_ASSERT_ALWAYS(
            frame != nullptr && "cannot find the given OpenSim::PhysicalFrame in the model");

        OpenSim::WrapObject& wrap_obj = add_wrap_object(*frame, std::move(wrap_obj_ptr));
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        model.set_selected(&wrap_obj);

        std::stringstream ss;
        ss << "added " << wrap_obj.getName();
        model.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to add a wrap object to the model"});
        return false;
    }
}

bool osc::action_add_wrap_object_to_geometry_path_wraps(
    ModelStatePair& model,
    const OpenSim::GeometryPath& geom_path,
    const OpenSim::WrapObject& wrap_object)
{
    if (model.is_readonly()) {
        return false;
    }

    try {
        OpenSim::Model& mut_model = model.upd_model();
        auto* mut_geom_path =
            find_component_mut<OpenSim::GeometryPath>(mut_model, geom_path.getAbsolutePath());
        OSC_ASSERT_ALWAYS(mut_geom_path != nullptr && "cannot find the geometry path in the model");
        auto* mut_wrap_object =
            find_component_mut<OpenSim::WrapObject>(mut_model, wrap_object.getAbsolutePath());
        OSC_ASSERT_ALWAYS(mut_wrap_object != nullptr && "cannot find wrap object in the model");

        std::stringstream msg;
        msg << "added " << mut_wrap_object->getName() << " to " << mut_geom_path->getName();

        mut_geom_path->addPathWrap(*mut_wrap_object);
        initialize_model(mut_model);
        initialize_state(mut_model);

        model.commit(std::move(msg).str());
        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{
                "error detected while trying to add a wrap object to a geometry path"});
        return false;
    }
}

bool osc::action_remove_wrap_object_from_geometry_path_wraps(
    ModelStatePair& model,
    const OpenSim::GeometryPath& geom_path,
    const OpenSim::WrapObject& wrap_object)
{
    if (model.is_readonly()) {
        return false;
    }

    // search for the wrap object in the geometry path's wrap list
    std::optional<int> index;
    for (int i = 0; i < geom_path.getWrapSet().getSize(); ++i) {
        if (geom_path.getWrapSet().get(i).getWrapObject() == &wrap_object) {
            index = i;
            break;
        }
    }

    if (!index) {
        log_info(
            "cannot find {} in {}: skipping deletion", wrap_object.getName(), geom_path.getName());
        return false;
    }

    try {
        OpenSim::Model& mut_model = model.upd_model();
        auto* mut_geom_path =
            find_component_mut<OpenSim::GeometryPath>(mut_model, geom_path.getAbsolutePath());
        OSC_ASSERT_ALWAYS(mut_geom_path != nullptr && "cannot find the geometry path in the model");
        auto* mut_wrap_object =
            find_component_mut<OpenSim::WrapObject>(mut_model, wrap_object.getAbsolutePath());
        OSC_ASSERT_ALWAYS(mut_wrap_object != nullptr && "cannot find wrap object in the model");

        std::stringstream msg;
        msg << "removed " << mut_wrap_object->getName() << " from " << mut_geom_path->getName();

        OSC_ASSERT(index);
        mut_geom_path->deletePathWrap(model.get_state(), *index);
        initialize_model(mut_model);
        initialize_state(mut_model);

        model.commit(std::move(msg).str());
        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{
                "error detected while trying to add a wrap object to a geometry path"});
        return false;
    }
}

bool osc::action_zero_all_coordinates(ModelStatePair& model)
{
    if (model.is_readonly()) {
        return false;
    }

    try {
        OpenSim::Model& mut_model = model.upd_model();
        for (auto& coordinate : mut_model.updComponentList<OpenSim::Coordinate>()) {
            const double range_min = min(coordinate.getRangeMin(), coordinate.getRangeMax());
            const double range_max = max(coordinate.getRangeMin(), coordinate.getRangeMax());
            coordinate.set_default_value(clamp(0.0, range_min, range_max));
        }
        initialize_model(mut_model);
        initialize_state(mut_model);
        model.commit("zeroed all coordinates");
        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while zeroing all coordinates in the model"});
        return false;
    }
}

bool osc::action_set_coordinate_speed(
    ModelStatePair& model, const OpenSim::Coordinate& coord, double new_speed)
{
    if (model.is_readonly()) {
        return false;
    }

    const OpenSim::ComponentPath coord_path = get_absolute_path(coord);

    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        auto* const mut_coord = find_component_mut<OpenSim::Coordinate>(mut_model, coord_path);
        if (not mut_coord) {
            // can't find the coordinate within the provided model
            model.set_model_version(old_version);
            return false;
        }

        // HACK: don't do a full model+state re-realization here: only do it
        //       when the caller wants to save the coordinate change
        mut_coord->setDefaultSpeedValue(new_speed);
        mut_coord->setSpeedValue(mut_model.updWorkingState(), new_speed);
        try_equilibrate_muscles_or_log_warning(mut_model, mut_model.updWorkingState());
        mut_model.realizeDynamics(mut_model.updWorkingState());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to set a coordinate's speed"});
        return false;
    }
}

bool osc::action_set_coordinate_speed_and_save(
    ModelStatePair& model, const OpenSim::Coordinate& coord, double new_speed)
{
    if (model.is_readonly()) {
        return false;
    }

    if (action_set_coordinate_speed(model, coord, new_speed)) {
        OpenSim::Model& mut_model = model.upd_model();
        initialize_model(mut_model);
        initialize_state(mut_model);

        std::stringstream ss;
        ss << "set " << coord.getName() << "'s speed";
        model.commit(std::move(ss).str());

        return true;
    } else {
        return false;  // the edit wasn't made
    }
}

bool osc::action_set_coordinate_locked_and_save(
    ModelStatePair& model, const OpenSim::Coordinate& coord, bool v)
{
    if (model.is_readonly()) {
        return false;
    }

    const OpenSim::ComponentPath coord_path = get_absolute_path(coord);

    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        auto* const mut_coord = find_component_mut<OpenSim::Coordinate>(mut_model, coord_path);
        if (not mut_coord) {
            // can't find the coordinate within the provided model
            model.set_model_version(old_version);
            return false;
        }

        mut_coord->setDefaultLocked(v);
        mut_coord->setLocked(mut_model.updWorkingState(), v);
        try_equilibrate_muscles_or_log_warning(mut_model, mut_model.updWorkingState());
        mut_model.realizeDynamics(mut_model.updWorkingState());

        std::stringstream ss;
        ss << (v ? "locked " : "unlocked ") << mut_coord->getName();
        model.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to lock a coordinate"});
        return false;
    }
}

// set the value of a coordinate, but don't save it to the model (yet)
bool osc::action_set_coordinate_value(
    ModelStatePair& model, const OpenSim::Coordinate& coord, double new_value)
{
    if (model.is_readonly()) {
        return false;
    }

    const OpenSim::ComponentPath coord_path = get_absolute_path(coord);

    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        auto* const mut_coord = find_component_mut<OpenSim::Coordinate>(mut_model, coord_path);
        if (not mut_coord) {
            // can't find the coordinate within the provided model
            model.set_model_version(old_version);
            return false;
        }

        const double range_min = min(mut_coord->getRangeMin(), mut_coord->getRangeMax());
        const double range_max = max(mut_coord->getRangeMin(), mut_coord->getRangeMax());

        if (not(range_min <= new_value and new_value <= range_max)) {
            // the requested edit is outside the coordinate's allowed range
            model.set_model_version(old_version);
            return false;
        }

        // HACK: don't do a full model+state re-realization here: only do it
        //       when the caller wants to save the coordinate change
        mut_coord->setDefaultValue(new_value);
        mut_coord->setValue(mut_model.updWorkingState(), new_value);
        initialize_model(mut_model);
        initialize_state(mut_model);

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to set a coordinate's value"});
        return false;
    }
}

bool osc::action_set_coordinate_value_and_save(
    ModelStatePair& model, const OpenSim::Coordinate& coord, double new_value)
{
    if (model.is_readonly()) {
        return false;
    }

    if (action_set_coordinate_value(model, coord, new_value)) {
        OpenSim::Model& mut_model = model.upd_model();

        // CAREFUL: ensure that *all* coordinate's default values are updated to reflect
        //          the current state
        //
        // You might be thinking "but, the caller only wanted to set one coordinate". You're
        // right, but OpenSim models can contain constraints where editing one coordinate causes
        // a bunch of other coordinates to change.
        //
        // See #345 for a longer explanation
        for (OpenSim::Coordinate& c : mut_model.updComponentList<OpenSim::Coordinate>()) {
            c.setDefaultValue(c.getValue(model.get_state()));
        }

        initialize_model(mut_model);
        initialize_state(mut_model);

        model.commit(
            std::format(
                "set {} to {}",
                coord.getName(),
                convert_coord_value_to_display_value(coord, new_value)));
        return true;
    } else {
        return false;  // an edit wasn't made
    }
}

bool osc::action_set_component_and_all_childrens_is_visible_to(
    ModelStatePair& model, const OpenSim::ComponentPath& path, bool new_visibility)
{
    if (model.is_readonly()) {
        return false;
    }

    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        OpenSim::Component* const mut_component = find_component_mut(mut_model, path);
        if (not mut_component) {
            // can't find the coordinate within the provided model
            model.set_model_version(old_version);
            return false;
        }

        try_set_appearance_property_is_visible_to(*mut_component, new_visibility);

        for (OpenSim::Component& c : mut_component->updComponentList()) {
            try_set_appearance_property_is_visible_to(c, new_visibility);
        }

        initialize_model(mut_model);
        initialize_state(mut_model);

        std::stringstream ss;
        ss << "set " << path.getComponentName() << " visibility to " << new_visibility;
        model.commit(std::move(ss).str());

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to hide a component"});
        return false;
    }
}

bool osc::action_show_only_component_and_all_children(
    ModelStatePair& model, const OpenSim::ComponentPath& path)
{
    if (model.is_readonly()) {
        return false;
    }

    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        OpenSim::Component* const mut_component = find_component_mut(mut_model, path);
        if (not mut_component) {
            // can't find the coordinate within the provided model
            model.set_model_version(old_version);
            return false;
        }

        // first, hide everything in the model
        for (OpenSim::Component& c : mut_model.updComponentList()) {
            try_set_appearance_property_is_visible_to(c, false);
        }

        // then show the intended component and its children
        try_set_appearance_property_is_visible_to(*mut_component, true);
        for (OpenSim::Component& c : mut_component->updComponentList()) {
            try_set_appearance_property_is_visible_to(c, true);
        }

        // reinitialize etc.
        initialize_model(mut_model);
        initialize_state(mut_model);

        // commit it
        {
            std::stringstream ss;
            ss << "showing only " << path.getComponentName();
            model.commit(std::move(ss).str());
        }

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to hide a component"});
        return false;
    }
}

bool osc::action_set_component_and_all_children_with_given_concrete_class_name_is_visible_to(
    ModelStatePair& model,
    const OpenSim::ComponentPath& root,
    std::string_view concrete_class_name,
    bool new_visibility)
{
    if (model.is_readonly()) {
        return false;
    }

    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        const OpenSim::Component* const mut_component = find_component_mut(mut_model, root);
        if (not mut_component) {
            // can't find the coordinate within the provided model
            model.set_model_version(old_version);
            return false;
        }

        // first, hide everything in the model
        for (OpenSim::Component& c : mut_model.updComponentList()) {
            if (c.getConcreteClassName() == concrete_class_name) {
                try_set_appearance_property_is_visible_to(c, new_visibility);
                for (OpenSim::Component& child : c.updComponentList()) {
                    try_set_appearance_property_is_visible_to(child, new_visibility);
                }
            }
        }

        // reinitialize etc.
        initialize_model(mut_model);
        initialize_state(mut_model);

        // commit it
        {
            std::stringstream ss;
            if (new_visibility) {
                ss << "showing ";
            } else {
                ss << "hiding ";
            }
            ss << concrete_class_name;
            model.commit(std::move(ss).str());
        }

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{
                "error detected while trying to show/hide components of a given type"});
        return false;
    }
}

bool osc::action_translate_station(
    ModelStatePair& model, const OpenSim::Station& station, const Vector3& delta_position)
{
    if (model.is_readonly()) {
        return false;
    }

    const OpenSim::ComponentPath station_path = get_absolute_path(station);
    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        auto* const mut_station = find_component_mut<OpenSim::Station>(mut_model, station_path);
        if (not mut_station) {
            model.set_model_version(old_version);  // the provided path isn't a station
            return false;
        }

        const SimTK::Vec3 original_pos = mut_station->get_location();
        const SimTK::Vec3 new_pos = original_pos + to<SimTK::Vec3>(delta_position);

        // perform mutation
        mut_station->set_location(new_pos);

        // HACK: don't perform a full model reinitialization because that would be very expensive
        // and it is very likely that it isn't necessary when dragging a station
        //
        // initialize_model(mutModel);  // don't do this
        initialize_state(mut_model);

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while trying to move a station"});
        return false;
    }
}

bool osc::action_translate_station_and_save(
    ModelStatePair& model, const OpenSim::Station& station, const Vector3& delta_position)
{
    if (model.is_readonly()) {
        return false;
    }

    if (action_translate_station(model, station, delta_position)) {
        OpenSim::Model& mut_model = model.upd_model();
        initialize_model(mut_model);
        initialize_state(mut_model);

        std::stringstream ss;
        ss << "translated " << station.getName();
        model.commit(std::move(ss).str());

        return true;
    } else {
        return false;  // edit wasn't made
    }
}

bool osc::action_translate_path_point(
    ModelStatePair& model, const OpenSim::PathPoint& path_point, const Vector3& delta_position)
{
    if (model.is_readonly()) {
        return false;
    }

    const OpenSim::ComponentPath pp_path = get_absolute_path(path_point);
    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        auto* const mut_path_point = find_component_mut<OpenSim::PathPoint>(mut_model, pp_path);
        if (not mut_path_point) {
            model.set_model_version(old_version);  // the provided path isn't a station
            return false;
        }

        const SimTK::Vec3 original_pos = mut_path_point->get_location();
        const SimTK::Vec3 new_pos = original_pos + to<SimTK::Vec3>(delta_position);

        // perform mutation
        mut_path_point->setLocation(new_pos);
        initialize_state(mut_model);

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to move a path point"});
        return false;
    }
}

bool osc::action_translate_path_point_and_save(
    ModelStatePair& model, const OpenSim::PathPoint& path_point, const Vector3& delta_position)
{
    if (model.is_readonly()) {
        return false;
    }

    if (action_translate_path_point(model, path_point, delta_position)) {
        OpenSim::Model& mut_model = model.upd_model();
        initialize_model(mut_model);
        initialize_state(mut_model);

        std::stringstream ss;
        ss << "translated " << path_point.getName();
        model.commit(std::move(ss).str());

        return true;
    } else {
        return false;  // edit wasn't made
    }
}

bool osc::action_transform_pof_v2(
    ModelStatePair& model,
    const OpenSim::PhysicalOffsetFrame& pof,
    const Vector3& new_translation,
    const EulerAngles& new_eulers)
{
    if (model.is_readonly()) {
        return false;
    }

    const OpenSim::ComponentPath pof_path = get_absolute_path(pof);
    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        auto* const mut_pof = find_component_mut<OpenSim::PhysicalOffsetFrame>(mut_model, pof_path);
        if (not mut_pof) {
            model.set_model_version(old_version);  // the provided path isn't a station
            return false;
        }

        // perform mutation
        mut_pof->set_translation(to<SimTK::Vec3>(new_translation));
        mut_pof->set_orientation(to<SimTK::Vec3>(new_eulers));
        initialize_model(mut_model);
        initialize_state(mut_model);

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to transform a POF"});
        return false;
    }
    return false;
}

bool osc::action_transform_wrap_object(
    ModelStatePair& model,
    const OpenSim::WrapObject& wo,
    const Vector3& delta_position,
    const EulerAngles& new_eulers)
{
    if (model.is_readonly()) {
        return false;
    }

    const OpenSim::ComponentPath pof_path = get_absolute_path(wo);
    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        auto* const mut_pof = find_component_mut<OpenSim::WrapObject>(mut_model, pof_path);
        if (not mut_pof) {
            model.set_model_version(old_version);  // the provided path isn't a station
            return false;
        }

        const SimTK::Vec3 original_pos = mut_pof->get_translation();
        const SimTK::Vec3 new_pos = original_pos + to<SimTK::Vec3>(delta_position);

        // perform mutation
        mut_pof->set_translation(new_pos);
        mut_pof->set_xyz_body_rotation(to<SimTK::Vec3>(new_eulers));
        initialize_model(mut_model);
        initialize_state(mut_model);

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to transform a POF"});
        return false;
    }
    return false;
}

bool osc::action_transform_contact_geometry(
    ModelStatePair& model,
    const OpenSim::ContactGeometry& contact_geom,
    const Vector3& delta_position,
    const EulerAngles& new_eulers)
{
    if (model.is_readonly()) {
        return false;
    }

    const OpenSim::ComponentPath pof_path = get_absolute_path(contact_geom);
    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();

        auto* const mut_geom = find_component_mut<OpenSim::ContactGeometry>(mut_model, pof_path);
        if (not mut_geom) {
            model.set_model_version(old_version);  // the provided path doesn't exist in the model
            return false;
        }

        const SimTK::Vec3 original_pos = mut_geom->get_location();
        const SimTK::Vec3 new_pos = original_pos + to<SimTK::Vec3>(delta_position);

        // perform mutation
        mut_geom->set_location(new_pos);
        mut_geom->set_orientation(to<SimTK::Vec3>(new_eulers));
        initialize_model(mut_model);
        initialize_state(mut_model);

        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to transform a POF"});
        return false;
    }
    return false;
}

bool osc::action_fit_sphere_to_mesh(ModelStatePair& model, const OpenSim::Mesh& open_sim_mesh)
{
    if (model.is_readonly()) {
        return false;
    }

    // fit a sphere to the mesh
    Sphere sphere;
    try {
        const Mesh mesh =
            to_osc_mesh_bake_scale_factors(model.get_model(), model.get_state(), open_sim_mesh);
        sphere = opyn::fit_sphere_htbad(mesh);
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to fit a sphere to a mesh"});
        return false;
    }

    // create an `OpenSim::OffsetFrame` expressed w.r.t. the same frame as the mesh that
    // places the origin-centered `OpenSim::Sphere` at the computed `origin`
    auto offset_frame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    offset_frame->setName("sphere_fit");
    offset_frame->connectSocket_parent(
        dynamic_cast<const OpenSim::PhysicalFrame&>(open_sim_mesh.getFrame()));
    offset_frame->setOffsetTransform(SimTK::Transform{to<SimTK::Vec3>(sphere.origin)});

    // create an origin-centered `OpenSim::Sphere` geometry to visually represent the computed
    // sphere
    auto open_sim_sphere = std::make_unique<OpenSim::Sphere>(sphere.radius);
    open_sim_sphere->setName("sphere_geom");
    open_sim_sphere->connectSocket_frame(*offset_frame);
    upd_appearance_to_fitted_geom(open_sim_sphere->upd_Appearance());

    // perform undoable model mutation
    const OpenSim::ComponentPath open_sim_mesh_path = get_absolute_path(open_sim_mesh);
    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();
        auto* const mut_open_sim_mesh =
            find_component_mut<OpenSim::Mesh>(mut_model, open_sim_mesh_path);
        if (not mut_open_sim_mesh) {
            model.set_model_version(old_version);  // the provided path doesn't exist in the model
            return false;
        }

        const std::string sphere_name = open_sim_sphere->getName();
        auto& pof_ref = add_model_component(mut_model, std::move(offset_frame));
        auto& sphere_ref = attach_geometry(pof_ref, std::move(open_sim_sphere));

        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        model.set_selected(&sphere_ref);

        std::stringstream ss;
        ss << "computed " << sphere_name;
        model.commit(std::move(ss).str());
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{
                "error detected while trying to add a sphere fit to the OpenSim model"});
        return false;
    }

    return true;
}

bool osc::action_fit_ellipsoid_to_mesh(ModelStatePair& model, const OpenSim::Mesh& open_sim_mesh)
{
    if (model.is_readonly()) {
        return false;
    }

    // fit an ellipsoid to the mesh
    Ellipsoid ellipsoid;
    try {
        const Mesh mesh =
            to_osc_mesh_bake_scale_factors(model.get_model(), model.get_state(), open_sim_mesh);
        ellipsoid = opyn::fit_ellipsoid_htbad(mesh);
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to fit an ellipsoid to a mesh"});
        return false;
    }

    // create an `OpenSim::OffsetFrame` expressed w.r.t. the same frame as the mesh that
    // places the origin-centered `OpenSim::Ellipsoid` at the computed ellipsoid's `origin`
    // and reorients the ellipsoid's XYZ along the computed ellipsoid directions
    //
    // (OSC note: `fit_ellipsoid_htbad` in OSC should yield a right-handed coordinate system)
    auto offset_frame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    offset_frame->setName("ellipsoid_fit");
    offset_frame->connectSocket_parent(
        dynamic_cast<const OpenSim::PhysicalFrame&>(open_sim_mesh.getFrame()));
    {
        // compute offset transform for ellipsoid
        SimTK::Mat33 m;
        auto directions = axis_directions_of(ellipsoid);
        m.col(0) = to<SimTK::Vec3>(directions[0]);
        m.col(1) = to<SimTK::Vec3>(directions[1]);
        m.col(2) = to<SimTK::Vec3>(directions[2]);
        const SimTK::Transform t{SimTK::Rotation{m}, to<SimTK::Vec3>(ellipsoid.origin)};
        offset_frame->setOffsetTransform(t);
    }

    // create an origin-centered `OpenSim::Ellipsoid` geometry to visually represent the computed
    // ellipsoid
    auto open_sim_ellipsoid = std::make_unique<OpenSim::Ellipsoid>(
        ellipsoid.radii[0], ellipsoid.radii[1], ellipsoid.radii[2]);
    open_sim_ellipsoid->setName("ellipsoid_geom");
    open_sim_ellipsoid->connectSocket_frame(*offset_frame);
    upd_appearance_to_fitted_geom(open_sim_ellipsoid->upd_Appearance());

    // mutate the model and add the relevant components
    const OpenSim::ComponentPath open_sim_mesh_path = get_absolute_path(open_sim_mesh);
    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();
        auto* const mut_open_sim_mesh =
            find_component_mut<OpenSim::Mesh>(mut_model, open_sim_mesh_path);
        if (not mut_open_sim_mesh) {
            model.set_model_version(old_version);  // the provided path doesn't exist in the model
            return false;
        }

        const std::string ellipsoid_name = open_sim_ellipsoid->getName();
        auto& pof_ref = add_model_component(mut_model, std::move(offset_frame));
        attach_geometry(pof_ref, std::move(open_sim_ellipsoid));

        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        model.set_selected(&pof_ref);

        std::stringstream ss;
        ss << "computed" << ellipsoid_name;
        model.commit(std::move(ss).str());
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{
                "error detected while trying to add a sphere fit to the OpenSim model"});
        return false;
    }

    return true;
}

bool osc::action_fit_plane_to_mesh(ModelStatePair& model, const OpenSim::Mesh& open_sim_mesh)
{
    if (model.is_readonly()) {
        return false;
    }

    // fit a plane to the mesh
    Plane plane;
    try {
        const Mesh mesh =
            to_osc_mesh_bake_scale_factors(model.get_model(), model.get_state(), open_sim_mesh);
        plane = opyn::fit_plane_htbad(mesh);
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to fit a plane to a mesh"});
        return false;
    }

    // create an `OpenSim::OffsetFrame` expressed w.r.t. the same frame as the mesh that
    // places the origin-centered `OpenSim::Brick` at the computed plane's `origin` and
    // also reorients the +1 in Y brick along the plane's normal
    auto offset_frame = std::make_unique<OpenSim::PhysicalOffsetFrame>();
    offset_frame->setName("plane_fit");
    offset_frame->connectSocket_parent(
        dynamic_cast<const OpenSim::PhysicalFrame&>(open_sim_mesh.getFrame()));
    {
        // +1Y in "brick space" should map to the plane's normal
        const Quaternion q = rotation({0.0f, 1.0f, 0.0f}, plane.normal);
        offset_frame->setOffsetTransform(
            SimTK::Transform{to<SimTK::Rotation>(q), to<SimTK::Vec3>(plane.origin)});
    }

    // create an origin-centered `OpenSim::Brick` geometry to visually represent the computed
    // plane
    // half-lengths are hard-coded, for now - the thin axis points along the normal
    auto open_sim_brick = std::make_unique<OpenSim::Brick>();
    open_sim_brick->set_half_lengths({0.2, 0.0005, 0.2});
    open_sim_brick->setName("plane_geom");
    open_sim_brick->connectSocket_frame(*offset_frame);
    upd_appearance_to_fitted_geom(open_sim_brick->upd_Appearance());

    // mutate the model and add the relevant components
    const OpenSim::ComponentPath open_sim_mesh_path = get_absolute_path(open_sim_mesh);
    const UID old_version = model.get_model_version();
    try {
        OpenSim::Model& mut_model = model.upd_model();
        auto* const mut_open_sim_mesh =
            find_component_mut<OpenSim::Mesh>(mut_model, open_sim_mesh_path);
        if (not mut_open_sim_mesh) {
            model.set_model_version(old_version);  // the provided path doesn't exist in the model
            return false;
        }

        const std::string fit_name = offset_frame->getName();
        auto& pof_ref = add_model_component(mut_model, std::move(offset_frame));
        attach_geometry(pof_ref, std::move(open_sim_brick));

        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);
        model.set_selected(&pof_ref);

        std::stringstream ss;
        ss << "computed " << fit_name;
        model.commit(std::move(ss).str());
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{
                "error detected while trying to add a sphere fit to the OpenSim model"});
        return false;
    }

    return true;
}

bool osc::action_import_landmarks(
    ModelStatePair& model,
    std::span<const opyn::NamedLandmark> landmarks,
    std::optional<std::string> maybe_name,
    std::optional<std::string> maybe_target_frame_abs_path)
{
    if (model.is_readonly()) {
        return false;
    }

    try {
        OpenSim::Model& mut_model = model.upd_model();

        OpenSim::PhysicalFrame* maybe_target_frame = nullptr;
        if (maybe_target_frame_abs_path) {
            auto* f =
                find_component_mut<OpenSim::PhysicalFrame>(mut_model, *maybe_target_frame_abs_path);
            if (f) {
                maybe_target_frame = f;
            } else {
                throw std::runtime_error(
                    std::format(
                        "Could not find the specified frame in the model: {}",
                        *maybe_target_frame_abs_path));
            }
        }

        for (const auto& landmark : landmarks) {
            if (maybe_target_frame) {
                // If the caller specified a target frame then the markers should be imported
                // as direct children of the target frame, rather than being dumped into the
                // generic markerset.
                add_component<OpenSim::Marker>(
                    *maybe_target_frame,
                    landmark.name,
                    *maybe_target_frame,
                    to<SimTK::Vec3>(landmark.position));
            } else {
                add_marker(
                    mut_model,
                    landmark.name,
                    mut_model.getGround(),
                    to<SimTK::Vec3>(landmark.position));
            }
        }
        finalize_connections(mut_model);
        initialize_model(mut_model);
        initialize_state(mut_model);

        std::stringstream ss;
        ss << "imported " << std::move(maybe_name).value_or("markers");
        model.commit(std::move(ss).str());
        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(
            std::runtime_error{"error detected while trying to import landmarks to the model"});
        return false;
    }
}

void osc::action_export_model_graph_to_dotviz(const std::shared_ptr<ModelStatePair>& model)
{
    App::upd().prompt_user_to_save_file_with_extension_async(
        [model](std::optional<std::filesystem::path> p) {
            if (not p) {
                return;  // user cancelled out of the prompt
            }

            if (std::ofstream of{*p}) {
                write_component_topology_graph_as_dot_viz(model->get_model(), of);
            } else {
                log_error("error opening {} for writing", p->string());
            }
        },
        "dot");
}

bool osc::action_export_model_graph_to_dotviz_clipboard(const OpenSim::Model& model)
{
    std::stringstream ss;
    write_component_topology_graph_as_dot_viz(model.getModel(), ss);
    set_clipboard_text(std::move(ss).str());
    return true;
}

bool osc::action_export_model_multibody_system_as_dotviz(const OpenSim::Model& model)
{
    std::stringstream ss;
    write_model_multibody_system_graph_as_dot_viz(model.getModel(), ss);
    set_clipboard_text(std::move(ss).str());
    return true;
}

bool osc::action_bake_station_defined_frames(ModelStatePair& model)
{
    if (model.is_readonly()) {
        return false;
    }

    // Ensure there is at least one `StationDefinedFrame`s in the model.
    {
        auto lst = model.get_model().getComponentList<OpenSim::StationDefinedFrame>();
        if (std::distance(lst.begin(), lst.end()) == 0) {
            return false;
        }
    }

    OpenSim::Model& mut_model = model.upd_model();
    bake_station_defined_frames(mut_model);
    model.commit("Bake `StationDefinedFrame`s");

    return true;
}

bool osc::action_move_marker_to_model_marker_set(
    ModelStatePair& model, const OpenSim::Marker& marker)
{
    if (model.is_readonly()) {
        return false;
    }

    const auto* owner = get_owner(marker);
    if (not owner) {
        return false;  // The marker is either the root (uhh) or disowned
    }

    if (dynamic_cast<const OpenSim::MarkerSet*>(owner) and
        get_owner<OpenSim::Model>(*owner) == &model.get_model()) {

        return false;  // The marker is already in the model's `MarkerSet`
    }

    // else: perform model mutation

    OpenSim::Model& mut_model = model.upd_model();
    OpenSim::Component* mut_owner = upd_owner(mut_model, marker);
    if (not mut_owner) {
        return false;  // Something went wrong trying to unlock/mutate the owner
    }
    auto* mut_marker = find_component_mut<OpenSim::Marker>(mut_model, marker.getAbsolutePath());
    if (not mut_marker) {
        return false;  // Something went wrong trying to unlock/mutate the original `Marker`
    }
    std::unique_ptr<OpenSim::Marker> extracted = mut_owner->extractComponent(mut_marker);
    if (not extracted) {
        return false;  // Something went wrong extracting the marker from its current owner
    }
    const auto* extracted_ptr = extracted.get();
    mut_model.addMarker(extracted.release());
    finalize_connections(mut_model);
    model.set_selected(extracted_ptr);
    initialize_model(mut_model);
    initialize_state(mut_model);
    std::stringstream msg;
    msg << "Moved " << extracted_ptr->getName() << " to /markerset";
    model.commit(std::move(msg).str());

    return true;
}

bool osc::action_translate_contact_hint(
    ModelStatePair& model,
    const OpenSim::Scholz2015GeometryPathObstacle& obstacle,
    const Vector3& delta_position)
{
    if (model.is_readonly()) {
        return false;
    }

    try {
        OpenSim::Model& mut_model = model.upd_model();
        auto* mut_obstacle = find_component_mut<OpenSim::Scholz2015GeometryPathObstacle>(
            mut_model, obstacle.getAbsolutePath());
        if (not mut_obstacle) {
            return false;  // Something went wrong trying to unlock/mutate the obstacle
        }
        mut_obstacle->set_contact_hint(
            mut_obstacle->get_contact_hint() + to<SimTK::Vec3>(delta_position));
        // Don't update state or reinitialize model: the wrapping requires a full rebuild, which
        // happens during `action_translate_contact_hint_and_save`.
        return true;
    } catch (const std::exception&) {
        std::throw_with_nested(std::runtime_error{"error detected while moving a contact hint"});
        return false;
    }
}

bool osc::action_translate_contact_hint_and_save(
    ModelStatePair& model,
    const OpenSim::Scholz2015GeometryPathObstacle& obstacle,
    const Vector3& delta_position)
{
    if (action_translate_contact_hint(model, obstacle, delta_position)) {
        OpenSim::Model& mut_model = model.upd_model();
        initialize_model(mut_model);
        initialize_state(mut_model);

        std::stringstream ss;
        ss << "translated " << obstacle.getName();
        model.commit(std::move(ss).str());

        return true;
    } else {
        return false;  // edit wasn't made
    }
}

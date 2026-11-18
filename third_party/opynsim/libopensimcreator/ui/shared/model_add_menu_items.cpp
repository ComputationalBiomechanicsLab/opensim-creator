#include "model_add_menu_items.h"

#include <libopensimcreator/documents/model/undoable_model_actions.h>
#include <libopensimcreator/ui/model_editor/add_body_popup.h>
#include <libopensimcreator/ui/model_editor/add_component_popup.h>
#include <libopensimcreator/ui/model_editor/select_1_pf_popup.h>
#include <libopensimcreator/ui/model_editor/select_component_popup.h>
#include <libopensimcreator/ui/model_editor/select_geometry_popup.h>
#include <libopensimcreator/ui/shared/basic_widgets.h>

#include <libopynsim/documents/model/model_state_pair.h>
#include <libopynsim/component_registry/component_registry.h>
#include <libopynsim/component_registry/static_component_registries.h>
#include <libopynsim/utilities/open_sim_helpers.h>
#include <liboscar/platform/app.h>
#include <liboscar/platform/widget.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/shims/cpp23/ranges.h>
#include <liboscar/ui/events/open_popup_event.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/utilities/c_string_view.h>
#include <liboscar/utilities/string_helpers.h>
#include <OpenSim/Common/Component.h>
#include <OpenSim/Common/ComponentPath.h>
#include <OpenSim/Simulation/Control/Controller.h>
#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PathActuator.h>
#include <OpenSim/Simulation/Model/Probe.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>

#include <sstream>
#include <utility>

class osc::ModelAddMenuItems::Impl final : public WidgetPrivate {
public:
    explicit Impl(Widget& owner, Widget* parent, std::shared_ptr<opyn::ModelStatePair> uum) :

        WidgetPrivate{owner, parent},
        model_{std::move(uum)}
    {}

    void set_target_parent_component(const OpenSim::ComponentPath& path)
    {
        maybe_target_parent_component_ = path;
    }

    void on_draw()
    {
        ui::push_id(this);

        const bool disabled = model_->is_readonly();
        if (disabled) {
            ui::begin_disabled();
        }

        ui::set_next_item_width(ui::get_content_region_available().x());
        draw_search_bar(search_string_);

        if (search_string_.empty()) {
            draw_default_component_list();
        } else {
            draw_search_results_or_no_results();
        }

        draw_target_component_specialized_adders();

        if (disabled) {
            ui::end_disabled();
        }

        ui::pop_id();
    }

private:
    void draw_target_component_specialized_adders()
    {
        const OpenSim::Component* component =
            opyn::find_component(model_->get_model(), maybe_target_parent_component_);
        if (not component) {
            return;
        }

        if (const auto* joint = dynamic_cast<const OpenSim::Joint*>(component)) {
            ui::draw_separator();
            draw_specialized_contextual_actions(*joint);
        } else if (const auto* hcf = dynamic_cast<const OpenSim::HuntCrossleyForce*>(component)) {
            ui::draw_separator();
            draw_specialized_contextual_actions(*hcf);
        } else if (const auto* pa = dynamic_cast<const OpenSim::PathActuator*>(component)) {
            ui::draw_separator();
            draw_specialized_contextual_actions(*pa);
        } else if (const auto* gp = dynamic_cast<const OpenSim::GeometryPath*>(component)) {
            ui::draw_separator();
            draw_specialized_contextual_actions(*gp);
        } else if (const auto* pf = dynamic_cast<const OpenSim::PhysicalFrame*>(component)) {
            ui::draw_separator();
            draw_specialized_contextual_actions(*pf);
        }
    }

    void draw_specialized_contextual_actions(const OpenSim::Joint& joint)
    {
        if (ui::draw_menu_item("Parent Offset Frame", {}, nullptr, model_->can_upd_model())) {
            action_add_parent_offset_frame_to_joint(*model_, joint.getAbsolutePath());
        }

        if (ui::draw_menu_item("Child Offset Frame", {}, nullptr, model_->can_upd_model())) {
            action_add_child_offset_frame_to_joint(*model_, joint.getAbsolutePath());
        }
    }

    void draw_specialized_contextual_actions(const OpenSim::HuntCrossleyForce& hcf)
    {
        if (not parent()) {
            return;  // can't open the select contact geometry popup.
        }
        if (opyn::size(hcf.get_contact_parameters()) > 1) {
            return;  // cannot edit: has more than one HuntCrossleyForce::Parameter
        }

        if (ui::draw_menu_item(
                "Associated Contact Geometry", {}, nullptr, model_->can_upd_model())) {

            const auto on_selection = [model = model_, path = hcf.getAbsolutePath()](
                                          const OpenSim::ComponentPath& geom_path) {
                action_assign_contact_geometry_to_hcf(*model, path, geom_path);
            };
            const auto filter = [](const OpenSim::Component& c) -> bool {
                return dynamic_cast<const OpenSim::ContactGeometry*>(&c) != nullptr;
            };
            auto popup = std::make_unique<SelectComponentPopup>(
                parent(), "Select Contact Geometry", model_, on_selection, filter);
            App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered(
            "Add Associated Contact Geometry",
            "Add an existing OpenSim::ContactGeometry in the model to this OpenSim::HuntCrossleyForce.\n\nCollisions are evaluated for all OpenSim::ContactGeometry attached to the OpenSim::HuntCrossleyForce. E.g. if you want an OpenSim::ContactSphere component to collide with an OpenSim::ContactHalfSpace component during a simulation then you should add both of those components to this force");
    }

    void draw_specialized_contextual_actions(const OpenSim::PathActuator& pa)
    {
        if (not parent()) {
            return;  // required in order to open a popup
        }

        if (ui::draw_menu_item("Path Point", {}, nullptr, model_->can_upd_model())) {
            auto on_selection = [model = model_, path = pa.getAbsolutePath()](
                                    const OpenSim::ComponentPath& pf_path) {
                action_add_path_point_to_path_actuator(*model, path, pf_path);
            };
            auto popup = std::make_unique<Select1PFPopup>(
                parent(), "Select Physical Frame", model_, on_selection);
            App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered(
            "Add Path Point",
            "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::GeometryPath");

        if (const auto* gp = dynamic_cast<const OpenSim::GeometryPath*>(&pa.getPath())) {
            if (ui::begin_menu("Path Wrap", model_->can_upd_model())) {
                draw_path_wrap_toggle_menu_items(*gp);
                ui::end_menu();
            }
        }
    }

    void draw_specialized_contextual_actions(const OpenSim::GeometryPath& geometry_path)
    {
        if (ui::begin_menu("Path Wrap", model_->can_upd_model())) {
            draw_path_wrap_toggle_menu_items(geometry_path);
            ui::end_menu();
        }
        if (ui::draw_menu_item("Path Point", {}, nullptr, model_->can_upd_model())) {
            auto on_selection = [model = model_, path = geometry_path.getAbsolutePath()](
                                    const OpenSim::ComponentPath& pf_path) {
                action_add_path_point_to_geometry_path(*model, path, pf_path);
            };
            auto popup = std::make_unique<Select1PFPopup>(
                parent(), "Select Physical Frame", model_, on_selection);
            App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered(
            "Add Path Point",
            "Add a new path point, attached to an OpenSim::PhysicalFrame in the model, to the end of the sequence of path points in this OpenSim::PathActuator");
    }

    void draw_specialized_contextual_actions(const OpenSim::PhysicalFrame& frame)
    {
        if (ui::draw_menu_item(
                "Geometry", {}, nullptr, model_->can_upd_model() and parent() != nullptr)) {

            const std::function<void(std::unique_ptr<OpenSim::Geometry>)> callback =
                [model = model_, path = frame.getAbsolutePath(), pptr = parent()](auto geom) {
                    action_attach_geometry_to_physical_frame(*model, path, std::move(geom));
                };
            auto popup = std::make_unique<SelectGeometryPopup>(
                parent(),
                "select geometry to attach",
                App::resource_filepath("OpenSimCreator/geometry"),
                callback);
            App::post_event<OpenPopupEvent>(*parent(), std::move(popup));
        }
        ui::draw_tooltip_if_item_hovered(
            "Add Geometry",
            "Add geometry to this component. Geometry can be removed by selecting it in the navigator and pressing DELETE");

        if (ui::draw_menu_item("Offset Frame", {}, nullptr, model_->can_upd_model())) {
            action_add_offset_frame_to_physical_frame(*model_, frame.getAbsolutePath());
        }
        ui::draw_tooltip_if_item_hovered(
            "Add Offset Frame",
            "Add an OpenSim::OffsetFrame as a child of this Component. Other components in the model can then connect to this OffsetFrame, rather than the base Component, so that it can connect at some offset that is relative to the parent Component");

        if (ui::begin_menu("Wrap Object", model_->can_upd_model())) {
            draw_add_wrap_objects_to_physical_frame_menu_items(frame.getAbsolutePath());
            ui::end_menu();
        }
    }

    void draw_path_wrap_toggle_menu_items(const OpenSim::GeometryPath& gp)
    {
        const auto wraps = opyn::get_all_wrap_objects_referenced_by(gp);
        for (const auto& wo : model_->get_model().getComponentList<OpenSim::WrapObject>()) {
            const bool enabled = cpp23::contains(wraps, &wo);

            ui::push_id(&wo);
            bool selected = enabled;
            if (ui::draw_menu_item(wo.getName(), {}, &selected, model_->can_upd_model())) {
                if (enabled) {
                    action_remove_wrap_object_from_geometry_path_wraps(*model_, gp, wo);
                } else {
                    action_add_wrap_object_to_geometry_path_wraps(*model_, gp, wo);
                }
            }
            ui::pop_id();
        }
    }

    void draw_add_wrap_objects_to_physical_frame_menu_items(
        const OpenSim::ComponentPath& physical_frame_abs_path)
    {
        // list each available `WrapObject` as something the user can add
        const auto& registry = opyn::get_component_registry<OpenSim::WrapObject>();
        for (const auto& entry : registry) {
            ui::push_id(&entry);
            if (ui::draw_menu_item(entry.name(), {}, nullptr, model_->can_upd_model())) {
                action_add_wrap_object_to_physical_frame(
                    *model_, physical_frame_abs_path, entry.instantiate());
            }
            ui::pop_id();
        }
    }

    void draw_default_component_list()
    {
        // action: add body
        {
            // draw button
            if (ui::draw_menu_item("Body", {}, nullptr, model_->can_upd_model())) {
                if (parent()) {
                    auto popup = std::make_unique<AddBodyPopup>(&owner(), "add body", model_);
                    App::post_event<OpenPopupEvent>(owner(), std::move(popup));
                }
            }

            // draw tooltip (if hovered)
            if (ui::is_item_hovered(ui::HoveredFlag::DelayNormal)) {
                ui::draw_tooltip(
                    "Add an OpenSim::Body into the model",
                    "An OpenSim::Body is a PhysicalFrame (reference frame) with an associated inertia specified by its mass, center-of-mass located in the PhysicalFrame, and its moment of inertia tensor about the center-of-mass");
            }
        }

        render_button(opyn::get_component_registry<OpenSim::Joint>());
        render_button(opyn::get_component_registry<OpenSim::ContactGeometry>());
        render_button(opyn::get_component_registry<OpenSim::Constraint>());
        render_button(opyn::get_component_registry<OpenSim::Force>());
        render_button(opyn::get_component_registry<OpenSim::Controller>());
        render_button(opyn::get_component_registry<OpenSim::Probe>());
        render_button(opyn::get_component_registry<OpenSim::Component>());
        render_button(opyn::get_opynsim_component_registry());
    }

    void draw_search_results_or_no_results()
    {
        bool search_result_fount = false;
        for (const auto& entry : opyn::get_all_registered_components()) {
            if (contains_case_insensitive(entry.name(), search_string_)) {
                if (ui::draw_menu_item(entry.name())) {
                    action_open_component_popup(entry);
                }
                search_result_fount = true;
            }
        }
        if (not search_result_fount) {
            ui::draw_text_disabled_and_centered("no results ");
            ui::same_line();
            if (ui::draw_small_button("clear search")) {
                search_string_.clear();
            }
        }
    }

    void render_button(const opyn::ComponentRegistryBase& registry)
    {
        if (ui::begin_menu(registry.name(), model_->can_upd_model())) {
            for (const auto& entry : registry) {
                if (ui::draw_menu_item(entry.name())) {
                    action_open_component_popup(entry);
                }

                if (ui::is_item_hovered(ui::HoveredFlag::DelayNormal)) {
                    ui::draw_tooltip(entry.name(), entry.description());
                }
            }

            ui::end_menu();
        }

        if (ui::is_item_hovered(ui::HoveredFlag::DelayNormal)) {
            ui::draw_tooltip(registry.name(), registry.description());
        }
    }

    void action_open_component_popup(const opyn::ComponentRegistryEntryBase& entry)
    {
        if (not parent()) {
            return;  // Can't fire popup-opening event upwards.
        }

        std::stringstream label;
        label << "Add " << entry.name();
        const OpenSim::Component* target =
            opyn::find_component(*model_, maybe_target_parent_component_);
        if (target) {
            label << " to " << target->getName();
        }

        auto popup = std::make_unique<AddComponentPopup>(
            &owner(), label.str(), model_, entry.instantiate(), maybe_target_parent_component_);
        App::post_event<OpenPopupEvent>(owner(), std::move(popup));
    }

    std::shared_ptr<opyn::ModelStatePair> model_;
    std::string search_string_;
    OpenSim::ComponentPath maybe_target_parent_component_;
};

osc::ModelAddMenuItems::ModelAddMenuItems(Widget* parent, std::shared_ptr<opyn::ModelStatePair> m) :
    Widget{std::make_unique<Impl>(*this, parent, std::move(m))}
{}

void osc::ModelAddMenuItems::set_target_parent_component(const OpenSim::ComponentPath& path)
{
    private_data().set_target_parent_component(path);
}

void osc::ModelAddMenuItems::impl_on_draw()
{
    private_data().on_draw();
}

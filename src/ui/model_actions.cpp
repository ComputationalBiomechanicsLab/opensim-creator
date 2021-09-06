#include "model_actions.hpp"

#include "src/Log.hpp"
#include "src/opensim_bindings/TypeRegistry.hpp"
#include "src/ui/AddBodyPopup.hpp"
#include "src/ui/select_2_pfs_popup.hpp"

#include <OpenSim/Simulation/Model/ContactGeometry.h>
#include <OpenSim/Simulation/Model/Force.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/Constraint.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <imgui.h>

using namespace osc;

osc::ui::model_actions::State::State() : abm{}, select_2_pfs{} {
}

static void draw_tooltip(char const* header, char const* description) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(header);
    ImGui::Dummy(ImVec2{0.0f, 1.0f});
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.7f, 0.7f, 0.7f, 1.0f});
    ImGui::TextUnformatted(description);
    ImGui::PopStyleColor();
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

static void render_actions_panel_content(
    osc::ui::model_actions::State& st,
    OpenSim::Model& model,
    std::function<void(OpenSim::Component*)> const& on_set_selection,
    std::function<void()> const& on_before_modify_model,
    std::function<void()> const& on_after_modify_model) {

    // action: add body
    {
        static constexpr char const* add_body_modal_name = "add body";

        // draw button
        if (ImGui::MenuItem("add body")) {
            ImGui::OpenPopup(add_body_modal_name);
        }

        // draw tooltip (if hovered)
        if (ImGui::IsItemHovered()) {
            draw_tooltip(
                "Add an OpenSim::Body into the model",
                "An OpenSim::Body is a PhysicalFrame (reference frame) with an associated inertia specified by its mass, center-of-mass located in the PhysicalFrame, and its moment of inertia tensor about the center-of-mass");
        }

        // draw popup (if requested)
        if (auto maybe_new_body = st.abm.draw(add_body_modal_name, model); maybe_new_body) {
            on_before_modify_model();
            model.addJoint(maybe_new_body->joint.release());
            auto* ptr = maybe_new_body->body.get();
            model.addBody(maybe_new_body->body.release());
            on_set_selection(const_cast<OpenSim::Body*>(ptr));
            on_after_modify_model();
        }
    }

    // action: add joint
    {
        bool open_popup = false;  // has to be outside ImGui::Menu

        // draw dropdown menu (containing concrete `OpenSim::ContactGeometry`s)
        if (ImGui::BeginMenu("add joint")) {
            auto names = JointRegistry::names();

            for (size_t i = 0; i < names.size(); ++i) {
                if (ImGui::MenuItem(names[i])) {
                    std::unique_ptr<OpenSim::Joint> copy{JointRegistry::prototypes()[i]->clone()};
                    st.add_component_popup = AddComponentPopup{std::move(copy)};
                    st.add_component_popup_name = "Add Joint";
                    open_popup = true;
                }
                if (ImGui::IsItemHovered()) {
                    draw_tooltip(names[i], JointRegistry::descriptions()[i]);
                }
            }

            ImGui::EndMenu();
        }

        // draw general OpenSim::Joint tooltip (if top-level menu hovered)
        if (ImGui::IsItemHovered()) {
            draw_tooltip(
                "Add an OpenSim::Joint into the model",
                "An OpenSim::Joint is a OpenSim::ModelComponent which connects two PhysicalFrames together and specifies their relative permissible motion as described in internal coordinates.");
        }

        // draw popup (if requested)
        if (open_popup) {
            ImGui::OpenPopup(st.add_component_popup_name);
        }
    }

    // action: add contact geometry
    {
        bool open_popup = false;  // has to be outside ImGui::Menu

        // draw dropdown menu (containing concrete `OpenSim::ContactGeometry`s)
        if (ImGui::BeginMenu("add contact geometry")) {
            auto names = ContactGeometryRegistry::names();

            for (size_t i = 0; i < names.size(); ++i) {
                if (ImGui::MenuItem(names[i])) {
                    std::unique_ptr<OpenSim::ContactGeometry> copy{ContactGeometryRegistry::prototypes()[i]->clone()};
                    st.add_component_popup = AddComponentPopup{std::move(copy)};
                    st.add_component_popup_name = "Add Contact Geometry";
                    open_popup = true;
                }
                if (ImGui::IsItemHovered()) {
                    draw_tooltip(names[i], ContactGeometryRegistry::descriptions()[i]);
                }
            }

            ImGui::EndMenu();
        }

        // draw general OpenSim::ContactGeometry tooltip (if top-level menu hovered)
        if (ImGui::IsItemHovered()) {
            draw_tooltip(
                "Add an OpenSim::ContactGeometry into the model",
                "Add a geometry with a physical shape that participates in contact modeling. The geometry is attached to an OpenSim::PhysicalFrame in the model (e.g. a body) and and moves with that frame.");
        }

        // draw popup (if requested)
        if (open_popup) {
            ImGui::OpenPopup(st.add_component_popup_name);
        }
    }

    // action: add constraint
    {
        bool open_popup = false;  // has to be outside ImGui::Menu

        // draw dropdown menu (containing concrete `OpenSim::Constraint`s)
        if (ImGui::BeginMenu("add constraint")) {
            auto names = ConstraintRegistry::names();
            for (size_t i = 0; i < names.size(); ++i) {

                if (ImGui::MenuItem(names[i])) {
                    std::unique_ptr<OpenSim::Constraint> copy{ConstraintRegistry::prototypes()[i]->clone()};
                    st.add_component_popup = AddComponentPopup{std::move(copy)};
                    st.add_component_popup_name = "Add Constraint";
                    open_popup = true;
                }
                if (ImGui::IsItemHovered()) {
                    draw_tooltip(names[i], ConstraintRegistry::descriptions()[i]);
                }
            }

            ImGui::EndMenu();
        }

        // draw general OpenSim::Constraint tooltip (if top-level menu hovered)
        if (ImGui::IsItemHovered()) {
            draw_tooltip(
                "Add an OpenSim::Constraint into the model",
                "Add a constraint into the model. A constraint typically constrains the motion of physical frame(s) in the model some way. For example, an OpenSim::ConstantDistanceConstraint constrains the system to *have* to keep two frames at some constant distance from eachover.");
        }

        if (open_popup) {
            ImGui::OpenPopup(st.add_component_popup_name);
        }
    }

    // action: add force
    {
        bool open_popup = false;  // has to be outside ImGui::Menu

        // draw dropdown menu (containing concrete `OpenSim::Force`s)
        if (ImGui::BeginMenu("add force/muscle")) {
            auto names = ForceRegistry::names();
            for (size_t i = 0; i < names.size(); ++i) {

                if (ImGui::MenuItem(names[i])) {
                    std::unique_ptr<OpenSim::Force> copy{ForceRegistry::prototypes()[i]->clone()};
                    st.add_component_popup = AddComponentPopup{std::move(copy)};
                    st.add_component_popup_name = "Add Force";
                    open_popup = true;
                }
                if (ImGui::IsItemHovered()) {
                    draw_tooltip(names[i], ForceRegistry::descriptions()[i]);
                }
            }

            ImGui::EndMenu();
        }

        // draw general OpenSim::Force tooltip (if top-level menu hovered)
        if (ImGui::IsItemHovered()) {
            draw_tooltip(
                "Add an OpenSim::Force into the model",
                "Add a force into the model. During a simulation, the force is applied to bodies or generalized coordinates in the model. Muscles are specialized `OpenSim::Force`s with biomech-focused features.");
        }


        if (open_popup) {
            ImGui::OpenPopup(st.add_component_popup_name);
        }
    }

    if (st.add_component_popup && st.add_component_popup_name) {
        auto new_component =
            st.add_component_popup->draw(st.add_component_popup_name, model);

        if (new_component) {
            auto ptr = new_component.get();

            if (dynamic_cast<OpenSim::Joint*>(new_component.get())) {
                on_before_modify_model();
                model.addJoint(static_cast<OpenSim::Joint*>(new_component.release()));
                on_set_selection(ptr);
                on_after_modify_model();
            } else if (dynamic_cast<OpenSim::Force*>(new_component.get())) {
                on_before_modify_model();
                model.addForce(static_cast<OpenSim::Force*>(new_component.release()));
                on_set_selection(ptr);
                on_after_modify_model();
            } else if (dynamic_cast<OpenSim::Constraint*>(new_component.get())) {
                on_before_modify_model();
                model.addConstraint(static_cast<OpenSim::Constraint*>(new_component.release()));
                on_set_selection(ptr);
                on_after_modify_model();
            } else if (dynamic_cast<OpenSim::ContactGeometry*>(new_component.get())) {
                on_before_modify_model();
                model.addContactGeometry(static_cast<OpenSim::ContactGeometry*>(new_component.release()));
                on_set_selection(ptr);
                on_after_modify_model();
            } else {
                log::error(
                    "don't know how to add a component of type %s to the model",
                    new_component->getConcreteClassName().c_str());
            }
        }
    }
}

void osc::ui::model_actions::draw(
    State& st,
    OpenSim::Model& model,
    std::function<void(OpenSim::Component*)> const& on_set_selection,
    std::function<void()> const& on_before_modify_model,
    std::function<void()> const& on_after_modify_model) {

    if (ImGui::BeginMenuBar()) {
        render_actions_panel_content(st, model, on_set_selection, on_before_modify_model, on_after_modify_model);
        ImGui::EndMenuBar();
    }
}

#include "model_actions_panel.hpp"

#include "src/application.hpp"
#include "src/log.hpp"
#include "src/opensim_bindings/type_registry.hpp"
#include "src/screens/show_model_screen.hpp"
#include "src/widgets/add_body_modal.hpp"
#include "src/widgets/add_joint_modal.hpp"
#include "src/widgets/select_2_pfs_modal.hpp"

#include <OpenSim/Simulation/Model/BushingForce.h>
#include <OpenSim/Simulation/Model/ContactHalfSpace.h>
#include <OpenSim/Simulation/Model/ContactMesh.h>
#include <OpenSim/Simulation/Model/ContactSphere.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/SimbodyEngine/BallJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/EllipsoidJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/GimbalJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PlanarJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PointOnLineConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/ScapulothoracicJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/SliderJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/UniversalJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/WeldJoint.h>
#include <imgui.h>

using namespace osmv;

Model_actions_panel_state::Model_actions_panel_state() :
    abm{},
    add_joint_modals{
        Add_joint_modal::create<OpenSim::FreeJoint>("FreeJoint"),
        Add_joint_modal::create<OpenSim::PinJoint>("PinJoint"),
        Add_joint_modal::create<OpenSim::UniversalJoint>("UniversalJoint"),
        Add_joint_modal::create<OpenSim::BallJoint>("BallJoint"),
        Add_joint_modal::create<OpenSim::EllipsoidJoint>("EllipsoidJoint"),
        Add_joint_modal::create<OpenSim::GimbalJoint>("GimbalJoint"),
        Add_joint_modal::create<OpenSim::PlanarJoint>("PlanarJoint"),
        Add_joint_modal::create<OpenSim::SliderJoint>("SliderJoint"),
        Add_joint_modal::create<OpenSim::WeldJoint>("WeldJoint"),
        Add_joint_modal::create<OpenSim::ScapulothoracicJoint>("ScapulothoracicJoint"),
    },
    select_2_pfs{} {
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
    Model_actions_panel_state& st,
    OpenSim::Model& model,
    std::function<void(OpenSim::Component*)> const& on_set_selection,
    std::function<void()> const& on_before_modify_model,
    std::function<void()> const& on_after_modify_model) {

    // simulate button
    {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4{0.0f, 1.0f, 0.0f, 1.0f});
        if (ImGui::MenuItem("simulate [Space]")) {
            auto copy = std::make_unique<OpenSim::Model>(model);
            Application::current().request_screen_transition<Show_model_screen>(std::move(copy));
        }
        ImGui::PopStyleColor();
    }

    // draw add body button
    {
        static constexpr char const* add_body_modal_name = "add body";

        if (ImGui::MenuItem("add body")) {
            ImGui::OpenPopup(add_body_modal_name);
        }

        // tooltip
        if (ImGui::IsItemHovered()) {
            draw_tooltip(
                "Add an OpenSim::Body to the model",
                "An OpenSim::Body is a PhysicalFrame (reference frame) with associated inertia specified by its mass, center-of-mass located in the PhysicalFrame, and its moment of inertia tensor about the center-of-mass");
        }

        auto on_body_add = [&](Added_body_modal_output out) {
            on_before_modify_model();
            model.addJoint(out.joint.release());
            OpenSim::Body const* b = out.body.get();
            model.addBody(out.body.release());
            on_set_selection(const_cast<OpenSim::Body*>(b));
            on_after_modify_model();
        };

        try_draw_add_body_modal(st.abm, add_body_modal_name, model, on_body_add);
    }

    // draw add joint dropdown
    {
        int joint_idx = -1;
        if (ImGui::BeginMenu("add joint")) {
            auto names = joint::names();
            for (size_t i = 0; i < names.size(); ++i) {
                if (ImGui::MenuItem(names[i])) {
                    joint_idx = static_cast<int>(i);
                }
                if (ImGui::IsItemHovered()) {
                    draw_tooltip(names[i], joint::descriptions()[i]);
                }
            }
            ImGui::EndMenu();
        }

        static constexpr char const* modal_name = "select joint pfs";
        if (joint_idx != -1) {
            st.joint_idx_for_pfs_popup = joint_idx;
            ImGui::OpenPopup(modal_name);
        }

        auto on_two_pfs_selected = [&](Select_2_pfs_modal_output out) {
            OSMV_ASSERT(
                st.joint_idx_for_pfs_popup >= 0 &&
                static_cast<size_t>(st.joint_idx_for_pfs_popup) < joint::prototypes().size());
            OpenSim::Joint const& prototype = *joint::prototypes()[static_cast<size_t>(st.joint_idx_for_pfs_popup)];
            std::unique_ptr<OpenSim::Joint> copy{prototype.clone()};
            copy->connectSocket_parent_frame(out.first);
            copy->connectSocket_child_frame(out.second);

            auto ptr = copy.get();
            on_before_modify_model();
            model.addJoint(copy.release());
            on_set_selection(ptr);
            on_after_modify_model();

            st.joint_idx_for_pfs_popup = -1;
        };

        draw_select_2_pfs_modal(st.select_2_pfs, modal_name, model, "parent", "child", on_two_pfs_selected);
    }

    if (ImGui::BeginMenu("add contact geometry")) {
        auto names = contact_geom::names();
        for (size_t i = 0; i < names.size(); ++i) {
            if (ImGui::MenuItem(names[i])) {
                std::unique_ptr<OpenSim::ContactGeometry> copy{contact_geom::prototypes()[i]->clone()};
                copy->setFrame(model.getGround());

                auto ptr = copy.get();
                on_before_modify_model();
                model.addContactGeometry(copy.release());
                model.finalizeConnections();
                on_set_selection(ptr);
                on_after_modify_model();
            }
            if (ImGui::IsItemHovered()) {
                draw_tooltip(names[i], contact_geom::descriptions()[i]);
            }
        }

        ImGui::EndMenu();
    }

    // draw add constraint dropdown
    {
        int constraintidx = -1;

        if (ImGui::BeginMenu("add constraint")) {
            auto names = constraint::names();
            for (size_t i = 0; i < names.size(); ++i) {
                if (ImGui::MenuItem(names[i])) {
                    constraintidx = static_cast<int>(i);
                }
                if (ImGui::IsItemHovered()) {
                    draw_tooltip(names[i], constraint::descriptions()[i]);
                }
            }

            ImGui::EndMenu();
        }

        static constexpr char const* modal_name = "select constraint frames";
        if (constraintidx != -1) {
            st.constraint_idx_for_pfs_popup = constraintidx;
            ImGui::OpenPopup(modal_name);
        }

        auto on_two_pfs_selected = [&](Select_2_pfs_modal_output out) {
            OSMV_ASSERT(
                st.constraint_idx_for_pfs_popup >= 0 &&
                static_cast<size_t>(st.constraint_idx_for_pfs_popup) < constraint::prototypes().size());
            OpenSim::Constraint const& prototype =
                *constraint::prototypes()[static_cast<size_t>(st.constraint_idx_for_pfs_popup)];
            std::unique_ptr<OpenSim::Constraint> copy{prototype.clone()};
            // copy->connectSocket_parent_frame(out.first);
            // copy->connectSocket_child_frame(out.second);

            auto ptr = copy.get();
            on_before_modify_model();
            model.addConstraint(copy.release());
            on_set_selection(ptr);
            on_after_modify_model();

            st.constraint_idx_for_pfs_popup = -1;
        };

        draw_select_2_pfs_modal(st.select_2_pfs, modal_name, model, "parent", "child", on_two_pfs_selected);
    }

    if (ImGui::BeginMenu("add force")) {
        auto names = force::names();
        for (size_t i = 0; i < names.size(); ++i) {
            if (ImGui::MenuItem(names[i])) {
                log::error("TODO");
            }
            if (ImGui::IsItemHovered()) {
                draw_tooltip(names[i], force::descriptions()[i]);
            }
        }

        ImGui::EndMenu();
    }

    // draw "Add PointOnLineConstraint" action (modal)
    {
        static constexpr char const* modal_name = "select pfs";

        if (ImGui::Button("Add PointOnLineConstraint")) {
            ImGui::OpenPopup(modal_name);
        }

        auto on_two_pfs_selected = [&](Select_2_pfs_modal_output out) {
            OpenSim::PhysicalFrame const& lineBody = out.first;
            SimTK::Vec3 lineDirection{0.0};
            SimTK::Vec3 pointOnLine{0.0};
            OpenSim::PhysicalFrame const& followerBody = out.second;
            SimTK::Vec3 followerPoint{0.0};

            auto polc = std::make_unique<OpenSim::PointOnLineConstraint>(
                lineBody, lineDirection, pointOnLine, followerBody, followerPoint);
            auto selection_ptr = polc.get();

            on_before_modify_model();
            model.addConstraint(polc.release());
            on_set_selection(selection_ptr);
            on_after_modify_model();
        };

        draw_select_2_pfs_modal(st.select_2_pfs, modal_name, model, "line_body", "follow_body", on_two_pfs_selected);
    }

    // draw "Add PointToPointSpring" action (modal)
    {
        static constexpr char const* modal_name = "select bodies to attatch p2p spring to";

        if (ImGui::Button("Add PointToPointSpring")) {
            ImGui::OpenPopup(modal_name);
        }

        auto on_two_pfs_selected = [&](Select_2_pfs_modal_output out) {
            OpenSim::PhysicalFrame const& body1 = out.first;
            SimTK::Vec3 point1 = {0.0, 0.0, 0.0};
            OpenSim::PhysicalFrame const& body2 = out.second;
            SimTK::Vec3 point2 = {0.0, 0.0, 0.0};
            double stiffness = 1.0;
            double restLength = 1.0;

            auto p2ps =
                std::make_unique<OpenSim::PointToPointSpring>(body1, point1, body2, point2, stiffness, restLength);
            auto selection_ptr = p2ps.get();

            on_before_modify_model();
            model.addForce(p2ps.release());
            on_set_selection(selection_ptr);
            on_after_modify_model();
        };

        draw_select_2_pfs_modal(st.select_2_pfs, modal_name, model, "body1", "body2", on_two_pfs_selected);
    }

    // draw "Add BrushingForce" action (modal)
    {
        static constexpr char const* modal_name = "select brushing force frames";

        if (ImGui::Button("Add BrushingForce")) {
            ImGui::OpenPopup(modal_name);
        }

        auto on_two_pfs_selected = [&](Select_2_pfs_modal_output out) {
            auto bf = std::make_unique<OpenSim::BushingForce>("bushing_force", out.first, out.second);
            auto selection_ptr = bf.get();

            on_before_modify_model();
            model.addForce(bf.release());
            on_set_selection(selection_ptr);
            on_after_modify_model();
        };

        draw_select_2_pfs_modal(st.select_2_pfs, modal_name, model, "frame1", "frame2", on_two_pfs_selected);
    }

    if (ImGui::Button("Add HuntCrossleyForce")) {
        auto hcf = std::make_unique<OpenSim::HuntCrossleyForce>();

        on_before_modify_model();
        model.addForce(hcf.release());
        on_after_modify_model();
    }
}

void osmv::draw_model_actions_panel(
    Model_actions_panel_state& st,
    OpenSim::Model& model,
    std::function<void(OpenSim::Component*)> const& on_set_selection,
    std::function<void()> const& on_before_modify_model,
    std::function<void()> const& on_after_modify_model) {

    if (ImGui::Begin("Actions", nullptr, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            render_actions_panel_content(st, model, on_set_selection, on_before_modify_model, on_after_modify_model);
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
}

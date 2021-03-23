#include "model_actions_panel.hpp"

#include "src/application.hpp"
#include "src/screens/show_model_screen.hpp"
#include "src/widgets/add_body_modal.hpp"
#include "src/widgets/add_joint_modal.hpp"
#include "src/widgets/select_2_pfs_modal.hpp"

#include <OpenSim/Simulation/Model/BushingForce.h>
#include <OpenSim/Simulation/Model/ContactHalfSpace.h>
#include <OpenSim/Simulation/Model/ContactSphere.h>
#include <OpenSim/Simulation/Model/HuntCrossleyForce.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PointToPointSpring.h>
#include <OpenSim/Simulation/SimbodyEngine/BallJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PinJoint.h>
#include <OpenSim/Simulation/SimbodyEngine/PointOnLineConstraint.h>
#include <OpenSim/Simulation/SimbodyEngine/UniversalJoint.h>
#include <imgui.h>

osmv::Model_actions_panel_state::Model_actions_panel_state() :
    abm{},
    add_joint_modals{Add_joint_modal::create<OpenSim::FreeJoint>("Add FreeJoint"),
                     Add_joint_modal::create<OpenSim::PinJoint>("Add PinJoint"),
                     Add_joint_modal::create<OpenSim::UniversalJoint>("Add UniversalJoint"),
                     Add_joint_modal::create<OpenSim::BallJoint>("Add BallJoint")},
    select_2_pfs{} {
}

void osmv::draw_model_actions_panel(
    Model_actions_panel_state& st,
    OpenSim::Model& model,
    std::function<void(OpenSim::Component*)> const& on_set_selection,
    std::function<void()> const& on_before_modify_model,
    std::function<void()> const& on_after_modify_model) {

    if (!ImGui::Begin("Actions")) {
        ImGui::End();
        return;
    }

    // draw "Add Body" action
    {
        static constexpr char const* add_body_modal_name = "add body";

        if (ImGui::Button("Add body")) {
            ImGui::OpenPopup(add_body_modal_name);
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

    // draw "Add Joint" actions (Add FreeJoint et. al.)
    {
        auto on_add_joint = [&](std::unique_ptr<OpenSim::Joint> joint) {
            OpenSim::Joint* ptr = joint.get();
            on_before_modify_model();
            model.addJoint(joint.release());
            on_set_selection(const_cast<OpenSim::Joint*>(ptr));
            on_after_modify_model();
        };

        for (Add_joint_modal& modal : st.add_joint_modals) {
            if (ImGui::Button(modal.modal_name.c_str())) {
                modal.show();
            }
            modal.draw(model, on_add_joint);
        }
    }

    // draw "Show model in viewer" action
    if (ImGui::Button("Show model in viewer")) {
        auto copy = std::make_unique<OpenSim::Model>(model);
        Application::current().request_screen_transition<Show_model_screen>(std::move(copy));
    }

    // draw "Add ContactSphere" action
    if (ImGui::Button("Add ContactSphere")) {
        auto cs = std::make_unique<OpenSim::ContactSphere>();
        cs->setFrame(model.getGround());

        on_before_modify_model();
        model.addContactGeometry(cs.release());
        model.finalizeConnections();
        on_after_modify_model();
    }

    // draw "Add ContactHalfSphere" action
    if (ImGui::Button("Add ContactHalfSpace")) {
        auto cs = std::make_unique<OpenSim::ContactHalfSpace>();
        cs->setFrame(model.getGround());

        on_before_modify_model();
        model.addContactGeometry(cs.release());
        model.finalizeConnections();
        on_after_modify_model();
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

    ImGui::End();
}

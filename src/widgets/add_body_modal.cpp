#include "add_body_modal.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <SimTKcommon.h>
#include <imgui.h>

#include <memory>

static constexpr char const modal_name[] = "add body";

void osmv::show_add_body_modal(Added_body_modal_state& st) {
    ImGui::OpenPopup(modal_name);
}

void osmv::try_draw_add_body_modal(
    Added_body_modal_state& st, OpenSim::Model& model, OpenSim::Component const** selection) {
    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (not ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    // collect user input
    ImGui::InputText("body name", st.body_name, sizeof(st.body_name));
    ImGui::InputFloat("mass", &st.mass);
    ImGui::InputFloat3("center of mass", st.com);
    ImGui::InputFloat3("inertia", st.inertia);

    ImGui::Text("join body to:");
    ImGui::Separator();
    ImGui::BeginChild("join", ImVec2{256.0f, 256.0f}, true, ImGuiWindowFlags_HorizontalScrollbar);
    for (OpenSim::PhysicalFrame const& pf : model.getComponentList<OpenSim::PhysicalFrame>()) {
        int styles_pushed = 0;
        if (&pf == st.selected_pf) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.3f, 1.0f, 0.3f, 1.0f});
            ++styles_pushed;
        }
        if (ImGui::Selectable(pf.getName().c_str())) {
            st.selected_pf = &pf;
        }
        ImGui::PopStyleColor(styles_pushed);
    }
    ImGui::EndChild();

    ImGui::Text("with a FreeJoint");

    ImGui::InputText("joint name", st.joint_name, sizeof(st.joint_name));
    ImGui::Checkbox("add offset frames to the joint", &st.add_offset_frames_to_the_joint);

    // add body (if user clicks `add` button)
    if (st.selected_pf != nullptr and ImGui::Button("add")) {
        // create user-requested body
        SimTK::Vec3 com{static_cast<double>(st.com[0]), static_cast<double>(st.com[1]), static_cast<double>(st.com[2])};
        SimTK::Inertia inertia{
            static_cast<double>(st.inertia[0]), static_cast<double>(st.inertia[1]), static_cast<double>(st.inertia[2])};
        OpenSim::Body* b = new OpenSim::Body{st.body_name, 1.0, com, inertia};
        model.addBody(b);

        // create a new (default) joint and assign it offset frames etc. accordingly
        OpenSim::FreeJoint* fj = nullptr;
        if (st.add_offset_frames_to_the_joint) {
            fj = new OpenSim::FreeJoint{};
            fj->setName(st.joint_name);

            // add first offset frame as joint's parent
            OpenSim::PhysicalOffsetFrame* pof1 = new OpenSim::PhysicalOffsetFrame{};
            pof1->setParentFrame(*st.selected_pf);
            pof1->setName(st.selected_pf->getName() + "_offset");
            fj->addFrame(pof1);
            fj->connectSocket_parent_frame(*pof1);

            // add second offset frame as joint's child
            OpenSim::PhysicalOffsetFrame* pof2 = new OpenSim::PhysicalOffsetFrame{};
            pof2->setParentFrame(*b);
            pof2->setName(b->getName() + "_offset");
            fj->addFrame(pof2);
            fj->connectSocket_child_frame(*pof2);
        } else {
            fj = new OpenSim::FreeJoint{st.joint_name, *st.selected_pf, *b};
        }
        model.addJoint(fj);

        *selection = b;
        st = {};  // reset user inputs

        ImGui::CloseCurrentPopup();
    }

    if (ImGui::Button("cancel")) {
        st = {};  // reset user inputs
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

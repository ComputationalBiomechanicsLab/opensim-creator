#include "add_joint_modal.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/Model/PhysicalFrame.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <imgui.h>

#include <algorithm>
#include <memory>
#include <string>

osmv::Add_joint_modal::Add_joint_modal(std::string _name, std::unique_ptr<OpenSim::Joint> _prototype) :
    modal_name{std::move(_name)},
    joint_prototype{std::move(_prototype)} {

    std::copy(std::begin(default_name), std::end(default_name), std::begin(added_joint_name));
}

void osmv::Add_joint_modal::reset() {
    parent_frame = nullptr;
    child_frame = nullptr;
    std::copy(std::begin(default_name), std::end(default_name), std::begin(modal_name));
}

void osmv::Add_joint_modal::show() {
    ImGui::OpenPopup(modal_name.c_str());
}

void osmv::Add_joint_modal::draw(OpenSim::Model& model, OpenSim::Component const** selected_component) {
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(512, 0));

    if (not ImGui::BeginPopupModal(modal_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::InputText("name", added_joint_name, sizeof(modal_name));

    ImGui::Columns(2);

    ImGui::Text("parent frame:");
    ImGui::BeginChild("parent", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
    // edge case: joint attaching to ground
    for (auto const& b : model.getComponentList<OpenSim::PhysicalFrame>()) {
        if (&b == child_frame) {
            continue;  // don't allow circular connections
        }
        int styles_pushed = 0;
        if (&b == parent_frame) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.3f, 1.0f, 0.3f, 1.0f});
            ++styles_pushed;
        }
        if (ImGui::Selectable(b.getName().c_str())) {
            parent_frame = &b;
        }
        ImGui::PopStyleColor(styles_pushed);
    }
    ImGui::EndChild();
    ImGui::NextColumn();

    ImGui::Text("child frame:");
    ImGui::BeginChild("child", ImVec2(256, 256), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (auto const& b : model.getComponentList<OpenSim::PhysicalFrame>()) {
        if (&b == parent_frame) {
            continue;  // don't allow circular connections
        }

        int styles_pushed = 0;
        if (&b == child_frame) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.3f, 1.0f, 0.3f, 1.0f});
            ++styles_pushed;
        }
        if (ImGui::Selectable(b.getName().c_str())) {
            child_frame = &b;
        }
        ImGui::PopStyleColor(styles_pushed);
    }
    ImGui::EndChild();
    ImGui::NextColumn();
    ImGui::NewLine();

    ImGui::Columns(1);

    if (parent_frame and child_frame) {
        if (ImGui::Button("OK")) {
            auto* joint = joint_prototype->clone();
            joint->setName(added_joint_name);

            SimTK::Vec3 offset{0.0, 0.0, 0.0};
            std::string parent_name = parent_frame->getName() + "_offset";
            auto* pof = new OpenSim::PhysicalOffsetFrame{parent_name, *parent_frame, offset};
            joint->addFrame(pof);
            joint->connectSocket_parent_frame(*pof);

            std::string child_name = child_frame->getName() + "_offset";
            auto* cof = new OpenSim::PhysicalOffsetFrame{child_name, *child_frame, offset};
            joint->addFrame(cof);
            joint->connectSocket_child_frame(*cof);

            model.addJoint(joint);
            *selected_component = joint;

            this->reset();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
    }

    if (ImGui::Button("Cancel")) {
        this->reset();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

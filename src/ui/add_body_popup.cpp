#include "add_body_popup.hpp"

#include "src/opensim_bindings/conversions.hpp"
#include "src/opensim_bindings/type_registry.hpp"
#include "src/ui/help_marker.hpp"
#include "src/ui/lockable_f3_editor.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <SimTKcommon.h>
#include <imgui.h>

#include <array>
#include <memory>

using namespace osc;

static std::unique_ptr<OpenSim::Joint>
    make_joint(osc::ui::add_body_popup::State& st, OpenSim::Body const& b, OpenSim::Joint const& joint_prototype) {

    std::unique_ptr<OpenSim::Joint> copy{joint_prototype.clone()};
    copy->setName(st.joint_name);

    if (!st.add_offset_frames_to_the_joint) {
        copy->connectSocket_parent_frame(*st.selected_pf);
        copy->connectSocket_child_frame(b);
    } else {
        // add first offset frame as joint's parent
        {
            auto pof1 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
            pof1->setParentFrame(*st.selected_pf);
            pof1->setName(st.selected_pf->getName() + "_offset");
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

std::optional<osc::ui::add_body_popup::New_body>
    osc::ui::add_body_popup::draw(State& st, char const* modal_name, OpenSim::Model const& model) {

    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // modal not showing
        return std::nullopt;
    }

    if (st.selected_pf == nullptr) {
        st.selected_pf = &model.getGround();
    }

    // collect user input
    ImGui::Columns(2);

    ImGui::Text("body name");
    ImGui::SameLine();
    ui::help_marker::draw("Name used to identify the OpenSim::Body in the model");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##bodyname", st.body_name, sizeof(st.body_name));
    ImGui::NextColumn();

    ImGui::Text("mass (kg)");
    ImGui::SameLine();
    ui::help_marker::draw("The mass of the body in kilograms");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputFloat("##mass", &st.mass);
    ImGui::NextColumn();

    ImGui::Text("center of mass");
    ImGui::SameLine();
    ui::help_marker::draw("The location (Vec3) of the mass center in the body frame");
    ImGui::NextColumn();
    ui::lockable_f3_editor::draw("##comlockbtn", "##comeditor", st.com, &st.com_locked);
    ImGui::NextColumn();

    ImGui::Text("inertia");
    ImGui::SameLine();
    ui::help_marker::draw(
        "The elements of the inertia tensor (Vec6) as [Ixx Iyy Izz Ixy Ixz Iyz] measured about the mass_center and not the body origin");
    ImGui::NextColumn();
    ui::lockable_f3_editor::draw("##inertialockbtn", "##intertiaeditor", st.inertia, &st.inertia_locked);
    ImGui::NextColumn();

    ImGui::Text("join body to");
    ImGui::SameLine();
    ui::help_marker::draw(
        "What the added body is joined to. Every OpenSim::Body in the model must be joined to another body in the Model. `ground` is a good default if you have no other bodies to connect to");
    ImGui::NextColumn();
    ImGui::BeginChild("join", ImVec2(0, 128.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
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
    ImGui::NextColumn();

    ImGui::Text("joint type");
    ImGui::SameLine();
    ui::help_marker::draw("The type of OpenSim::Joint that will connect the new OpenSim::Body to the selection above");
    ImGui::NextColumn();
    {
        auto names = osc::joint::names();
        ImGui::Combo("##jointtype", &st.joint_idx, names.data(), static_cast<int>(names.size()));
    }
    ImGui::NextColumn();

    ImGui::Text("joint name");
    ImGui::SameLine();
    ui::help_marker::draw("The name of the OpenSim::Joint specified above");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##jointnameinput", st.joint_name, sizeof(st.joint_name));
    ImGui::NextColumn();

    ImGui::Text("add offset frames?");
    ImGui::SameLine();
    ui::help_marker::draw(
        "Whether osc should automatically add intermediate offset frames to the OpenSim::Joint. A joint can attach to the the two bodies (this added one, plus the selected one) directly. However, most model designs have the joint attach to offset frames which, themselves, attach to the bodies. The utility of this is that the offset frames can later be moved/adjusted.");
    ImGui::NextColumn();
    ImGui::Checkbox("##addoffsetframescheckbox", &st.add_offset_frames_to_the_joint);
    ImGui::NextColumn();

    ImGui::Text("geometry");
    ImGui::SameLine();
    ui::help_marker::draw(
        "Visual geometry attached to this body. This is what the OpenSim::Body looks like in the UI. The geometry is purely cosmetic and does not affect the simulation");
    ImGui::NextColumn();
    {
        static constexpr char const* attach_modal_name = "addbody_attachgeometry";
        char const* label = st.attach_geom.selected ? st.attach_geom.selected->get_mesh_file().c_str() : "attach";
        if (ImGui::Button(label)) {
            ImGui::OpenPopup(attach_modal_name);
        }

        if (auto attached = ui::attach_geometry_popup::draw(st.attach_geom.state, attach_modal_name); attached) {
            st.attach_geom.selected = std::move(attached);
        }
    }
    ImGui::NextColumn();

    ImGui::Columns();
    ImGui::Dummy(ImVec2{0.0f, 1.0f});

    if (ImGui::Button("cancel")) {
        st = {};  // reset user inputs
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();

    std::optional<New_body> rv = std::nullopt;
    if (ImGui::Button("add")) {

        // create user-requested body
        auto com = stk_vec3_from(st.com);
        auto inertia = stk_inertia_from(st.inertia);
        auto body = std::make_unique<OpenSim::Body>(st.body_name, 1.0, com, inertia);
        auto joint = make_joint(st, *body, *osc::joint::prototypes()[static_cast<size_t>(st.joint_idx)]);

        if (st.attach_geom.selected) {
            body->attachGeometry(st.attach_geom.selected.release());
        }

        rv = New_body{std::move(body), std::move(joint)};
        st = {};  // reset user inputs
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return rv;
}

#include "add_body_modal.hpp"

#include "src/opensim_bindings/conversions.hpp"
#include "src/opensim_bindings/type_registry.hpp"
#include "src/widgets/help_marker.hpp"
#include "src/widgets/lockable_f3_editor.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <SimTKcommon.h>
#include <imgui.h>

#include <array>
#include <memory>

using namespace osmv;

static std::unique_ptr<OpenSim::Joint>
    make_joint(Added_body_modal_state& st, OpenSim::Body const& b, OpenSim::Joint const& joint_prototype) {

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

void osmv::try_draw_add_body_modal(
    Added_body_modal_state& st,
    char const* modal_name,
    OpenSim::Model const& model,
    std::function<void(Added_body_modal_output)> const& on_add_requested) {

    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // modal not showing
        return;
    }

    if (st.selected_pf == nullptr) {
        st.selected_pf = &model.getGround();
    }

    // collect user input
    ImGui::Columns(2);

    ImGui::Text("body name");
    ImGui::SameLine();
    draw_help_marker("Name used to identify the OpenSim::Body in the model");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##bodyname", st.body_name, sizeof(st.body_name));
    ImGui::NextColumn();

    ImGui::Text("mass (kg)");
    ImGui::SameLine();
    draw_help_marker("The mass of the body in kilograms");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputFloat("##mass", &st.mass);
    ImGui::NextColumn();

    ImGui::Text("center of mass");
    ImGui::SameLine();
    draw_help_marker("The location (Vec3) of the mass center in the body frame");
    ImGui::NextColumn();
    draw_lockable_f3_editor("##comlockbtn", "##comeditor", st.com, &st.com_locked);
    ImGui::NextColumn();

    ImGui::Text("inertia");
    ImGui::SameLine();
    draw_help_marker(
        "The elements of the inertia tensor (Vec6) as [Ixx Iyy Izz Ixy Ixz Iyz] measured about the mass_center and not the body origin");
    ImGui::NextColumn();
    draw_lockable_f3_editor("##inertialockbtn", "##intertiaeditor", st.inertia, &st.inertia_locked);
    ImGui::NextColumn();

    ImGui::Text("join body to");
    ImGui::SameLine();
    draw_help_marker(
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
    draw_help_marker("The type of OpenSim::Joint that will connect the new OpenSim::Body to the selection above");
    ImGui::NextColumn();
    {
        auto names = osmv::joint::names();
        ImGui::Combo("##jointtype", &st.joint_idx, names.data(), static_cast<int>(names.size()));
    }
    ImGui::NextColumn();

    ImGui::Text("joint name");
    ImGui::SameLine();
    draw_help_marker("The name of the OpenSim::Joint specified above");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##jointnameinput", st.joint_name, sizeof(st.joint_name));
    ImGui::NextColumn();

    ImGui::Text("add offset frames?");
    ImGui::SameLine();
    draw_help_marker(
        "Whether osmv should automatically add intermediate offset frames to the OpenSim::Joint. A joint can attach to the the two bodies (this added one, plus the selected one) directly. However, most model designs have the joint attach to offset frames which, themselves, attach to the bodies. The utility of this is that the offset frames can later be moved/adjusted.");
    ImGui::NextColumn();
    ImGui::Checkbox("##addoffsetframescheckbox", &st.add_offset_frames_to_the_joint);
    ImGui::NextColumn();

    ImGui::Text("geometry");
    ImGui::SameLine();
    draw_help_marker(
        "Visual geometry attached to this body. This is what the OpenSim::Body looks like in the UI. The geometry is purely cosmetic and does not affect the simulation");
    ImGui::NextColumn();
    {
        static constexpr char const* attach_modal_name = "addbody_attachgeometry";
        char const* label = st.attach_geom.selected ? st.attach_geom.selected->get_mesh_file().c_str() : "attach";
        if (ImGui::Button(label)) {
            ImGui::OpenPopup(attach_modal_name);
        }
        draw_attach_geom_modal_if_opened(
            st.attach_geom.state,
            attach_modal_name,
            [& selected = st.attach_geom.selected](std::unique_ptr<OpenSim::Mesh> m) { selected = std::move(m); });
    }
    ImGui::NextColumn();

    ImGui::Columns();
    ImGui::Dummy(ImVec2{0.0f, 1.0f});

    if (ImGui::Button("cancel")) {
        st = {};  // reset user inputs
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();

    if (ImGui::Button("add")) {

        // create user-requested body
        auto com = stk_vec3_from(st.com);
        auto inertia = stk_inertia_from(st.inertia);
        auto body = std::make_unique<OpenSim::Body>(st.body_name, 1.0, com, inertia);
        auto joint = make_joint(st, *body, *osmv::joint::prototypes()[static_cast<size_t>(st.joint_idx)]);

        if (st.attach_geom.selected) {
            body->attachGeometry(st.attach_geom.selected.release());
        }

        on_add_requested(Added_body_modal_output{std::move(body), std::move(joint)});
        st = {};  // reset user inputs
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

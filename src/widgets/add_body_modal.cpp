#include "add_body_modal.hpp"

#include "src/opensim_bindings/conversions.hpp"
#include "src/utils/indirect_ptr.hpp"
#include "src/utils/indirect_ref.hpp"
#include "src/widgets/lockable_f3_editor.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Body.h>
#include <OpenSim/Simulation/SimbodyEngine/FreeJoint.h>
#include <SimTKcommon.h>
#include <imgui.h>

#include <memory>

void osmv::show_add_body_modal(Added_body_modal_state& st) {
    ImGui::OpenPopup(st.modal_name);
}

static void prettify_coord_names(OpenSim::FreeJoint& fj) {
    static constexpr std::array<const char*, 6> const lut = {"rx", "ry", "rz", "tx", "ty", "tz"};

    for (size_t i = 0; i < lut.size(); ++i) {
        auto& coord = fj.upd_coordinates(static_cast<int>(i));
        coord.setName(lut[i] + coord.getName());
    }
}

static std::unique_ptr<OpenSim::Joint> make_joint(osmv::Added_body_modal_state& st, OpenSim::Body const& b) {
    if (not st.add_offset_frames_to_the_joint) {
        auto fj = std::make_unique<OpenSim::FreeJoint>(st.joint_name, *st.selected_pf, b);
        prettify_coord_names(*fj);
        return std::move(fj);
    }

    auto fj = std::make_unique<OpenSim::FreeJoint>();
    fj->setName(st.joint_name);
    prettify_coord_names(*fj);

    // add first offset frame as joint's parent
    {
        auto pof1 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        pof1->setParentFrame(*st.selected_pf);
        pof1->setName(st.selected_pf->getName() + "_offset");
        fj->addFrame(pof1.get());
        fj->connectSocket_parent_frame(*pof1.release());
    }

    // add second offset frame as joint's child
    {
        auto pof2 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
        pof2->setParentFrame(b);
        pof2->setName(b.getName() + "_offset");
        fj->addFrame(pof2.get());
        fj->connectSocket_child_frame(*pof2.release());
    }

    return std::move(fj);
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void osmv::try_draw_add_body_modal(
    Added_body_modal_state& st, Indirect_ref<OpenSim::Model>& model, Indirect_ptr<OpenSim::Component>& selection) {

    if (st.selected_pf == nullptr) {
        st.selected_pf = &model.get().getGround();
    }

    // OpenSim::Model& model, OpenSim::Component const** selection
    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (not ImGui::BeginPopupModal(st.modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // modal not showing
        return;
    }

    // collect user input
    ImGui::Columns(2);

    ImGui::Text("body name");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##bodyname", st.body_name, sizeof(st.body_name));
    ImGui::NextColumn();

    ImGui::Text("mass (unitless)");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputFloat("##mass", &st.mass);
    ImGui::NextColumn();

    ImGui::Text("center of mass");
    ImGui::NextColumn();
    draw_lockable_f3_editor("##comlockbtn", "##comeditor", st.com, &st.com_locked);
    ImGui::NextColumn();

    ImGui::Text("inertia");
    ImGui::NextColumn();
    draw_lockable_f3_editor("##inertialockbtn", "##intertiaeditor", st.inertia, &st.inertia_locked);
    ImGui::NextColumn();

    ImGui::Text("join body to");
    ImGui::SameLine();
    HelpMarker("OpenSim::Body's must joined to something else with a joint");
    ImGui::NextColumn();
    ImGui::BeginChild("join", ImVec2(0, 128.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (OpenSim::PhysicalFrame const& pf : model.get().getComponentList<OpenSim::PhysicalFrame>()) {
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
    HelpMarker("The type of OpenSim::Joint that will connect the new OpenSim::Body to whatever was selected above");
    ImGui::NextColumn();
    ImGui::Text("OpenSim::FreeJoint");
    ImGui::NextColumn();

    ImGui::Text("joint name");
    ImGui::SameLine();
    HelpMarker("The name of the OpenSim::Joint specified above");
    ImGui::NextColumn();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##jointnameinput", st.joint_name, sizeof(st.joint_name));
    ImGui::NextColumn();

    ImGui::Text("add offset frames?");
    ImGui::SameLine();
    HelpMarker(
        "Whether osmv should automatically add intermediate offset frames to the OpenSim::Joint (rather than joining directly)");
    ImGui::NextColumn();
    ImGui::Checkbox("##addoffsetframescheckbox", &st.add_offset_frames_to_the_joint);
    ImGui::NextColumn();

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

        // hold onto this pointer so that it can be used post-release
        OpenSim::Body const* bptr = body.get();

        {
            auto guard = model.modify();
            guard->addBody(body.release());
            guard->addJoint(make_joint(st, *bptr).release());
        }

        selection.reset(bptr);
        st = {};  // reset user inputs

        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

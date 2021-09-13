#include "AddBodyPopup.hpp"

#include "src/OpenSimBindings/TypeRegistry.hpp"
#include "src/SimTKBindings/SimTKConverters.hpp"
#include "src/UI/HelpMarker.hpp"
#include "src/UI/F3Editor.hpp"
#include "src/Styling.hpp"

#include <OpenSim/Simulation/Model/Model.h>
#include <OpenSim/Simulation/SimbodyEngine/Joint.h>
#include <SimTKcommon.h>
#include <imgui.h>

#include <array>
#include <memory>

using namespace osc;

namespace {

    // create a "standard" OpenSim::Joint
    std::unique_ptr<OpenSim::Joint> makeJoint(
            AddBodyPopup& st,
            OpenSim::Body const& b,
            OpenSim::Joint const& jointPrototype) {

        std::unique_ptr<OpenSim::Joint> copy{jointPrototype.clone()};
        copy->setName(st.jointName);

        if (!st.addOffsetFramesToTheJoint) {
            copy->connectSocket_parent_frame(*st.selectedPF);
            copy->connectSocket_child_frame(b);
        } else {
            // add first offset frame as joint's parent
            {
                auto pof1 = std::make_unique<OpenSim::PhysicalOffsetFrame>();
                pof1->setParentFrame(*st.selectedPF);
                pof1->setName(st.selectedPF->getName() + "_offset");
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
}


// public API

std::optional<NewBody> osc::AddBodyPopup::draw(char const* modalName, OpenSim::Model const& model) {

    auto& st = *this;

    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(modalName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // modal not showing
        return std::nullopt;
    }

    // coerce selected joint pf to ground if user hasn't selected anything
    if (st.selectedPF == nullptr) {
        st.selectedPF = &model.getGround();
    }

    ImGui::Columns(2);

    // prompt name
    {
        ImGui::Text("body name");
        ImGui::SameLine();
        DrawHelpMarker("The name used to identify the OpenSim::Body in the model. OpenSim typically uses the name to identify connections between components in a model, so the name should be unique.");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##bodyname", st.bodyName, sizeof(st.bodyName));
        ImGui::NextColumn();
    }

    // prompt mass
    {
        ImGui::Text("mass (kg)");
        ImGui::SameLine();
        DrawHelpMarker("The mass of the body in kilograms");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputFloat("##mass", &st.mass);
        ImGui::NextColumn();
    }

    // prompt center of mass
    {
        ImGui::Text("center of mass");
        ImGui::SameLine();
        DrawHelpMarker("The location of the mass center in the body frame.");
        ImGui::NextColumn();
        DrawF3Editor("##comlockbtn", "##comeditor", st.com, &st.comLocked);
        ImGui::NextColumn();
    }

    // prompt inertia
    {
        ImGui::Text("inertia (tensor)");
        ImGui::SameLine();
        DrawHelpMarker("The elements of the inertia tensor (Vec6) as [Ixx Iyy Izz Ixy Ixz Iyz]. These are measured about the center of mass, *not* the center of the body frame.");
        ImGui::NextColumn();
        DrawF3Editor("##inertialockbtn", "##intertiaeditor", st.inertia, &st.inertiaLocked);
        ImGui::NextColumn();
    }

    // prompt body/ground that new body will connect to (via a joint)
    {
        ImGui::Text("join to");
        ImGui::SameLine();
        DrawHelpMarker("What the added body will be joined to. All bodies in an OpenSim model are connected to other bodies, or the ground, by joints. This is true even if the joint is unconstrained and does nothing (e.g. an OpenSim::FreeJoint) or if the joint constrains motion in all direcctions (e.g. an OpenSim::WeldJoint).");
        ImGui::NextColumn();

        ImGui::BeginChild("join targets", ImVec2(0, 128.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (OpenSim::PhysicalFrame const& pf : model.getComponentList<OpenSim::PhysicalFrame>()) {
            if (ImGui::Selectable(pf.getName().c_str(), &pf == st.selectedPF)) {
                st.selectedPF = &pf;
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();
    }

    // prompt joint type for the above
    {
        ImGui::Text("joint type");
        ImGui::SameLine();
        DrawHelpMarker("The type of OpenSim::Joint that will connect the new OpenSim::Body to the selection above");
        ImGui::NextColumn();
        {
            auto names = osc::JointRegistry::names();
            ImGui::Combo("##jointtype", &st.jointIdx, names.data(), static_cast<int>(names.size()));
        }
        ImGui::NextColumn();
    }

    // prompt joint name
    {
        ImGui::Text("joint name");
        ImGui::SameLine();
        DrawHelpMarker("The name of the OpenSim::Joint that will join the new body to the existing frame specified above");
        ImGui::NextColumn();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##jointnameinput", st.jointName, sizeof(st.jointName));
        ImGui::NextColumn();
    }

    // prompt adding offset frames
    {
        ImGui::Text("add offset frames");
        ImGui::SameLine();
        DrawHelpMarker("Whether osc should automatically add intermediate offset frames to the OpenSim::Joint. A joint can attach to the two bodies (this added one, plus the selected one) directly. However, many OpenSim model designs instead make the joint attach to offset frames which, themselves, attach to the bodies. The utility of doing this is that the offset frames can be manually adjusted later, rather than *having* to attach the center of the joint to the center of the body");
        ImGui::NextColumn();
        ImGui::Checkbox("##addoffsetframescheckbox", &st.addOffsetFramesToTheJoint);
        ImGui::NextColumn();

    }

    // prompt geometry
    {
        // TODO: doesn't populate the list by default anymore

        ImGui::Text("geometry");
        ImGui::SameLine();
        DrawHelpMarker("Attaches visual geometry to the new body. This is what the OpenSim::Body looks like in the UI. The geometry is purely cosmetic and does not affect the simulation");
        ImGui::NextColumn();
        {
            static constexpr char const* attach_modal_name = "addbody_attachgeometry";

            char const* label = "attach";
            if (st.attach_geom.selected) {
                OpenSim::Geometry const& attached = *st.attach_geom.selected;
                if (OpenSim::Mesh const* mesh = dynamic_cast<OpenSim::Mesh const*>(&attached); mesh) {
                    label = mesh->getGeometryFilename().c_str();
                } else {
                    label = attached.getConcreteClassName().c_str();
                }
            }

            if (ImGui::Button(label)) {
                ImGui::OpenPopup(attach_modal_name);
            }

            if (auto attached = st.attach_geom.state.draw(attach_modal_name); attached) {
                st.attach_geom.selected = std::move(attached);
            }
        }
        ImGui::NextColumn();
    }

    ImGui::Columns();

    // end of input prompting

    ImGui::Dummy(ImVec2{0.0f, 1.0f});

    std::optional<NewBody> rv = std::nullopt;

    // show cancel button
    if (ImGui::Button("cancel")) {

        st = AddBodyPopup{};  // reset user inputs
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    // show add button
    if (ImGui::Button(ICON_FA_PLUS " add body")) {
        // create user-requested body
        auto comStk = SimTKVec3FromV3(st.com);
        auto inertiaStk = SimTKInertiaFromV3(st.inertia);
        auto body = std::make_unique<OpenSim::Body>(st.bodyName, 1.0, comStk, inertiaStk);
        auto joint = makeJoint(st, *body, *osc::JointRegistry::prototypes()[static_cast<size_t>(st.jointIdx)]);

        if (st.attach_geom.selected) {
            body->attachGeometry(st.attach_geom.selected.release());
        }

        rv = NewBody{std::move(body), std::move(joint)};

        st = AddBodyPopup{};  // reset user inputs
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return rv;
}

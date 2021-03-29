#include "add_component_popup.hpp"

#include "src/assertions.hpp"
#include "src/widgets/help_marker.hpp"
#include "src/widgets/properties_editor.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>

#include <memory>
#include <vector>

using namespace osmv;

static std::vector<OpenSim::AbstractSocket const*> get_pf_sockets(OpenSim::Component& c) {
    std::vector<OpenSim::AbstractSocket const*> rv;
    for (std::string name : c.getSocketNames()) {
        OpenSim::AbstractSocket const& sock = c.getSocket(name);
        if (sock.getConnecteeTypeName() == "PhysicalFrame") {
            rv.push_back(&sock);
        }
    }
    return rv;
}

struct Add_component_popup::Impl final {
    std::unique_ptr<OpenSim::Component> proto;

    // prop editor
    Properties_editor_state prop_editor;

    // socket assignments
    std::vector<OpenSim::AbstractSocket const*> pf_sockets = get_pf_sockets(*proto);
    std::vector<OpenSim::PhysicalFrame const*> pf_conectee_choices =
        std::vector<OpenSim::PhysicalFrame const*>(pf_sockets.size());

    Impl(std::unique_ptr<OpenSim::Component> p) : proto{std::move(p)} {
    }

    [[nodiscard]] bool already_assigned(OpenSim::PhysicalFrame const* pf) const noexcept {
        auto it =
            std::find_if(pf_conectee_choices.begin(), pf_conectee_choices.end(), [&](auto* el) { return el == pf; });
        return it != pf_conectee_choices.end();
    }

    [[nodiscard]] bool all_sockets_assigned() const noexcept {
        return std::all_of(
            pf_conectee_choices.begin(), pf_conectee_choices.end(), [](auto* ptr) { return ptr != nullptr; });
    }
};

Add_component_popup::Add_component_popup(std::unique_ptr<OpenSim::Component> prototype) :
    impl{new Impl{std::move(prototype)}} {
}

Add_component_popup::Add_component_popup(Add_component_popup&&) = default;
Add_component_popup& Add_component_popup::operator=(Add_component_popup&&) = default;
Add_component_popup::~Add_component_popup() noexcept = default;

std::unique_ptr<OpenSim::Component> Add_component_popup::draw(char const* modal_name, OpenSim::Model const& model) {
    // center the modal
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(512, 0));
    }

    // try to show modal
    if (!ImGui::BeginPopupModal(modal_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // modal not showing
        return nullptr;
    }
    // else: draw modal content

    // prop editor
    ImGui::Text("Properties");
    ImGui::SameLine();
    draw_help_marker(
        "These are properties of the OpenSim::Component being added. Their datatypes, default values, and help text are defined in the source code (see OpenSim_DECLARE_PROPERTY in OpenSim's C++ source code, if you want the details). Their default values are typically sane enough to let you add the component directly into your model.");

    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    draw_properties_editor(impl->prop_editor, *impl->proto, []() {}, []() {});
    ImGui::Dummy(ImVec2(0.0f, 1.0f));

    // draw socket choices
    //
    // it's assumed that sockets *must* be assigned, because the model can be left in an invalid
    // state if a socket is left unassigned
    if (!impl->pf_sockets.empty()) {
        ImGui::Text("Socket assignments (required)");
        ImGui::SameLine();
        draw_help_marker(
            "The OpenSim::Component being added has `socket`s that connect to other components in the model. You must specify what these sockets should be connected to; otherwise, the component cannot be added to the model.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ImGui::Separator();
        ImGui::Columns(2);

        OSMV_ASSERT(impl->pf_sockets.size() == impl->pf_conectee_choices.size());

        for (size_t i = 0; i < impl->pf_sockets.size(); ++i) {
            OpenSim::AbstractSocket const& sock = *impl->pf_sockets[i];
            OpenSim::PhysicalFrame const** current_choice = &impl->pf_conectee_choices[i];

            ImGui::Text("%s", sock.getName().c_str());
            ImGui::NextColumn();

            ImGui::PushID(static_cast<int>(i));
            ImGui::BeginChild("##pfselector", ImVec2{ImGui::GetContentRegionAvail().x, 128.0f});
            for (auto const& b : model.getComponentList<OpenSim::PhysicalFrame>()) {
                bool selected = &b == *current_choice;
                if (ImGui::Selectable(b.getName().c_str(), selected)) {
                    *current_choice = &b;
                }
            }
            ImGui::EndChild();
            ImGui::PopID();
            ImGui::NextColumn();
        }

        ImGui::Dummy(ImVec2(0.0f, 1.0f));
    }

    if (auto* pa = dynamic_cast<OpenSim::PathActuator*>(impl->proto.get()); pa) {
        ImGui::Text("Path Points (at least 2 required)");
        ImGui::SameLine();
        draw_help_marker(
            "The Component being added is (effectively) a line that connects physical frames (e.g. bodies) in the model. For example, an OpenSim::Muscle can be described as an actuator that connects bodies in the model together. You **must** specify at least two physical frames on the line in order to add a PathActuator component.\n\nDetails: in OpenSim, some `Components` are `PathActuator`s. All `Muscle`s are defined as `PathActuator`s. A `PathActuator` is an `Actuator` that actuates along a path. Therefore, a `Model` containing a `PathActuator` with zero or one points would be invalid. This is why it is required that you specify at least two points");
        ImGui::Separator();

        ImGui::Text("TODO: let user select N `OpenSim::PhysicalFrame`s in the model");

        ImGui::Dummy(ImVec2(0.0f, 1.0f));
    }

    std::unique_ptr<OpenSim::Component> rv = nullptr;
    if (impl->all_sockets_assigned()) {
        if (ImGui::Button("ok")) {
            // clone the prototype into the return value
            rv.reset(impl->proto->clone());

            // assign sockets
            for (size_t i = 0; i < impl->pf_conectee_choices.size(); ++i) {
                rv->updSocket(impl->pf_sockets[i]->getName()).connect(*impl->pf_conectee_choices[i]);
            }

            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
    }

    if (ImGui::Button("cancel")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();

    return rv;
}

#include "add_component_popup.hpp"

#include "src/assertions.hpp"
#include "src/ui/help_marker.hpp"
#include "src/ui/properties_editor.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>

#include <memory>
#include <vector>

using namespace osc;
using namespace osc::ui;

std::vector<OpenSim::AbstractSocket const*> osc::ui::add_component_popup::get_pf_sockets(OpenSim::Component& c) {
    std::vector<OpenSim::AbstractSocket const*> rv;
    for (std::string name : c.getSocketNames()) {
        OpenSim::AbstractSocket const& sock = c.getSocket(name);
        if (sock.getConnecteeTypeName() == "PhysicalFrame") {
            rv.push_back(&sock);
        }
    }
    return rv;
}

[[nodiscard]] static bool all_sockets_assigned(add_component_popup::State const& st) noexcept {
    return std::all_of(st.physframe_connectee_choices.begin(), st.physframe_connectee_choices.end(), [](auto* ptr) {
        return ptr != nullptr;
    });
}

std::unique_ptr<OpenSim::Component>
    osc::ui::add_component_popup::draw(State& st, char const* modal_name, OpenSim::Model const& model) {

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
    help_marker::draw(
        "These are properties of the OpenSim::Component being added. Their datatypes, default values, and help text are defined in the source code (see OpenSim_DECLARE_PROPERTY in OpenSim's C++ source code, if you want the details). Their default values are typically sane enough to let you add the component directly into your model.");

    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 1.0f));
    {
        auto maybe_updater = ui::properties_editor::draw(st.prop_editor, *st.prototype);
        if (maybe_updater) {
            maybe_updater->updater(const_cast<OpenSim::AbstractProperty&>(maybe_updater->prop));
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));

    // draw socket choices
    //
    // it's assumed that sockets *must* be assigned, because the model can be left in an invalid
    // state if a socket is left unassigned
    if (!st.physframe_sockets.empty()) {
        ImGui::Text("Socket assignments (required)");
        ImGui::SameLine();
        help_marker::draw(
            "The OpenSim::Component being added has `socket`s that connect to other components in the model. You must specify what these sockets should be connected to; otherwise, the component cannot be added to the model.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ImGui::Separator();
        ImGui::Columns(2);

        OSC_ASSERT(st.physframe_sockets.size() == st.physframe_connectee_choices.size());

        for (size_t i = 0; i < st.physframe_sockets.size(); ++i) {
            OpenSim::AbstractSocket const& sock = *st.physframe_sockets[i];
            OpenSim::PhysicalFrame const** current_choice = &st.physframe_connectee_choices[i];

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

    if (auto* pa = dynamic_cast<OpenSim::PathActuator*>(st.prototype.get()); pa) {
        ImGui::Text("Path Points (at least 2 required)");
        ImGui::SameLine();
        help_marker::draw(
            "The Component being added is (effectively) a line that connects physical frames (e.g. bodies) in the model. For example, an OpenSim::Muscle can be described as an actuator that connects bodies in the model together. You **must** specify at least two physical frames on the line in order to add a PathActuator component.\n\nDetails: in OpenSim, some `Components` are `PathActuator`s. All `Muscle`s are defined as `PathActuator`s. A `PathActuator` is an `Actuator` that actuates along a path. Therefore, a `Model` containing a `PathActuator` with zero or one points would be invalid. This is why it is required that you specify at least two points");
        ImGui::Separator();

        ImGui::Text("TODO: let user select N `OpenSim::PhysicalFrame`s in the model");

        ImGui::Dummy(ImVec2(0.0f, 1.0f));
    }

    std::unique_ptr<OpenSim::Component> rv = nullptr;
    if (all_sockets_assigned(st)) {
        if (ImGui::Button("ok")) {
            // clone the prototype into the return value
            rv.reset(st.prototype->clone());

            // assign sockets
            for (size_t i = 0; i < st.physframe_connectee_choices.size(); ++i) {
                rv->updSocket(st.physframe_sockets[i]->getName()).connect(*st.physframe_connectee_choices[i]);
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

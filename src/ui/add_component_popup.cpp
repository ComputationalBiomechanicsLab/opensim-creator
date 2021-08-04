#include "add_component_popup.hpp"

#include "src/assertions.hpp"
#include "src/ui/help_marker.hpp"
#include "src/ui/properties_editor.hpp"
#include "src/styling.hpp"

#include <OpenSim/Common/Component.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <imgui.h>

#include <memory>
#include <vector>
#include <sstream>

using namespace osc;
using namespace osc::ui;

[[nodiscard]] static bool all_sockets_assigned(add_component_popup::State const& st) noexcept {
    return std::all_of(st.pf_connectees.begin(), st.pf_connectees.end(), [](auto* ptr) {
        return ptr != nullptr;
    });
}

// public API

std::vector<OpenSim::AbstractSocket const*> osc::ui::add_component_popup::get_pf_sockets(OpenSim::Component& c) {

    std::vector<OpenSim::AbstractSocket const*> rv;
    for (std::string const& name : c.getSocketNames()) {
        OpenSim::AbstractSocket const& sock = c.getSocket(name);
        if (sock.getConnecteeTypeName() == "PhysicalFrame") {
            rv.push_back(&sock);
        }
    }

    return rv;
}

std::unique_ptr<OpenSim::Component> osc::ui::add_component_popup::draw(
        State& st,
        char const* modal_name,
        OpenSim::Model const& model) {


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

    // draw name editor
    {
        ImGui::Columns(2);

        ImGui::TextUnformatted("name");
        ImGui::SameLine();
        help_marker::draw("Name the newly-added component will have after being added into the model. Note: this is used to derive the name of subcomponents (e.g. path points)");
        ImGui::NextColumn();

        char buf[128]{};
        std::strncpy(buf, st.name.c_str(), sizeof(buf) - 1);
        if (ImGui::InputText("##componentname", buf, sizeof(buf))) {
            st.name = buf;
        }
        ImGui::NextColumn();

        ImGui::Columns();
    }


    // draw property editor
    //
    // this lets the user edit the prototype's properties before adding it into the model
    {
        ImGui::TextUnformatted("Properties");
        ImGui::SameLine();
        help_marker::draw(
            "These are properties of the OpenSim::Component being added. Their datatypes, default values, and help text are defined in the source code (see OpenSim_DECLARE_PROPERTY in OpenSim's C++ source code, if you want the details). Their default values are typically sane enough to let you add the component directly into your model.");
        ImGui::Separator();

        ImGui::Dummy(ImVec2(0.0f, 1.0f));

        auto maybe_updater = ui::properties_editor::draw(st.prop_editor, *st.prototype);
        if (maybe_updater) {
            maybe_updater->updater(const_cast<OpenSim::AbstractProperty&>(maybe_updater->prop));
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));

    // draw socket editor (if it has sockets)
    //
    // this lets the user assign connectees to each of the prototype's sockets
    //
    // *required*: many OpenSim components are considered invalid if they have
    //             disconnected sockets. Therefore, this popup requires assigning
    //             all sockets before it will let the user have the prototype
    if (!st.pf_sockets.empty()) {
        ImGui::TextUnformatted("Socket assignments (required)");
        ImGui::SameLine();
        help_marker::draw(
            "The OpenSim::Component being added has `socket`s that connect to other components in the model. You must specify what these sockets should be connected to; otherwise, the component cannot be added to the model.\n\nIn OpenSim, a Socket formalizes the dependency between a Component and another object (typically another Component) without owning that object. While Components can be composites (of multiple components) they often depend on unrelated objects/components that are defined and owned elsewhere. The object that satisfies the requirements of the Socket we term the 'connectee'. When a Socket is satisfied by a connectee we have a successful 'connection' or is said to be connected.");
        ImGui::Separator();

        ImGui::Dummy(ImVec2(0.0f, 1.0f));

        // lhs: socket name, rhs: connectee choices
        ImGui::Columns(2);

        // for each socket in the prototype (cached), check if the user has chosen a
        // connectee for it yet and provide a UI for selecting them
        for (size_t i = 0; i < st.pf_sockets.size(); ++i) {
            OpenSim::AbstractSocket const& sock = *st.pf_sockets[i];
            OpenSim::PhysicalFrame const*& connectee = st.pf_connectees[i];

            // lhs: socket name
            ImGui::TextUnformatted(sock.getName().c_str());
            ImGui::NextColumn();

            // rhs: connectee choices
            ImGui::PushID(static_cast<int>(i));
            ImGui::BeginChild("##pfselector", ImVec2{ImGui::GetContentRegionAvail().x, 128.0f});

            // edge-case: also ensure that `connectee` is *somewhere* in the
            // model arugment. If it isn't, we might have a stale pointer that
            // should be nulled out
            bool found = false;

            // iterate through PFs in model and print them out
            for (auto const& b : model.getComponentList<OpenSim::PhysicalFrame>()) {
                bool is_selected = &b == connectee;

                if (is_selected) {
                    found = true;
                }

                if (ImGui::Selectable(b.getName().c_str(), is_selected)) {
                    connectee = &b;
                    found = true;
                }
            }

            if (!found) {
                connectee = nullptr;
            }

            ImGui::EndChild();
            ImGui::PopID();
            ImGui::NextColumn();
        }
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));

    // draw path point selector (if it's an OpenSim::PathActuator)
    OpenSim::PathActuator* prototype_as_pa =
            dynamic_cast<OpenSim::PathActuator*>(st.prototype.get());
    if (prototype_as_pa) {
        ImGui::TextUnformatted("Path Points (at least 2 required)");
        ImGui::SameLine();
        help_marker::draw(
            "The Component being added is (effectively) a line that connects physical frames (e.g. bodies) in the model. For example, an OpenSim::Muscle can be described as an actuator that connects bodies in the model together. You **must** specify at least two physical frames on the line in order to add a PathActuator component.\n\nDetails: in OpenSim, some `Components` are `PathActuator`s. All `Muscle`s are defined as `PathActuator`s. A `PathActuator` is an `Actuator` that actuates along a path. Therefore, a `Model` containing a `PathActuator` with zero or one points would be invalid. This is why it is required that you specify at least two points");
        ImGui::Separator();

        // show list of choices
        ImGui::BeginChild("##pf_ppchoices", ImVec2{ImGui::GetContentRegionAvail().x, 128.0f});

        // edge-case set all entries as missing
        //
        // we are going to iterate through all PFs in the model anyway, so we can use that
        // iteration to set them as not missing. Doing this lets us know whether we have a
        // stale pointer that should be deleted
        for (State::Path_point& pp : st.pps) {
            pp.missing = true;
        }

        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{66.0f/255.0f, 150.0f/255.0f, 250.0f/255.0f, 92.0f/255.0f});
        for (auto const& b : model.getComponentList<OpenSim::PhysicalFrame>()) {
            auto it = std::find_if(st.pps.begin(), st.pps.end(), [&](auto const& pp) {
                return pp.ptr == &b;
            });

            bool is_selected = it != st.pps.end();

            if (is_selected) {
                it->missing = false;
            }

            if (ImGui::Selectable(b.getName().c_str(), is_selected)) {
                // toggle, or add
                if (is_selected) {
                    it->ptr = nullptr;
                } else {
                    State::Path_point pp;
                    pp.ptr = &b;
                    pp.missing = false;
                    st.pps.push_back(pp);
                }
            }
        }
        ImGui::PopStyleColor();

        // edge-case (again): remove missing/null entries, so that the user
        // choice list is valid
        {
            auto it = std::remove_if(st.pps.begin(), st.pps.end(), [](State::Path_point const& pp) {
                return pp.ptr == nullptr || pp.missing;
            });
            st.pps.erase(it, st.pps.end());
        }

        ImGui::EndChild();
    }

    ImGui::Dummy(ImVec2(0.0f, 1.0f));

    // != nullptr if user clicks "OK"
    std::unique_ptr<OpenSim::Component> rv = nullptr;

    // draw "cancel" button
    if (ImGui::Button("cancel")) {
        ImGui::CloseCurrentPopup();
    }

    // figure out if the user can add the new Component yet

    bool has_name = !st.name.empty();
    bool all_socks_assigned = all_sockets_assigned(st);
    bool has_enough_pathpoints = !prototype_as_pa || st.pps.size() >= 2;

    // draw "add" button (if the user has performed all necessary steps)
    if (has_name && all_socks_assigned && has_enough_pathpoints) {

        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS " add")) {
            // clone the prototype into the return value
            rv.reset(st.prototype->clone());

            // assign name
            rv->setName(st.name);

            // assign sockets to connectees
            for (size_t i = 0; i < st.pf_connectees.size(); ++i) {
                OpenSim::AbstractSocket const& socket = *st.pf_sockets[i];
                OpenSim::PhysicalFrame const& connectee = *st.pf_connectees[i];

                rv->updSocket(socket.getName()).connect(connectee);
            }

            // assign path points, if it's an OpenSim::PathActuator
            if (prototype_as_pa) {
                OpenSim::PathActuator* rv_as_pa = dynamic_cast<OpenSim::PathActuator*>(rv.get());
                OSC_ASSERT(st.pps.size() >= 2 && "incorrect number of path points: this should be checked by the UI before allowing the user to click 'ok'");

                for (size_t i = 0; i < st.pps.size(); ++i) {
                    State::Path_point const& pp = st.pps[i];
                    OSC_ASSERT(pp.ptr != nullptr && "cannot assign a nullptr to a path actuator, the UI should have checked this");

                    // naming convention in previous OpenSim models is (e.g.): TRIlong-P1
                    std::stringstream name;
                    name << rv_as_pa->getName() << "-P" << i+1;

                    // user should just edit the locations in the main UI: doing it through
                    // this modal would get messy very quickly
                    SimTK::Vec3 pos{0.0, 0.0, 0.0};

                    rv_as_pa->addNewPathPoint(std::move(name).str(), *pp.ptr, pos);
                }
            }

            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::EndPopup();

    return rv;
}

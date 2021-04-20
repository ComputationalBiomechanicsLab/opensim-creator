#pragma once

#include "src/ui/properties_editor.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace OpenSim {
    class Component;
    class Model;
    class AbstractSocket;
    class PhysicalFrame;
}

namespace osc::ui::add_component_popup {
    std::vector<OpenSim::AbstractSocket const*> get_pf_sockets(OpenSim::Component& c);

    struct State final {
        std::unique_ptr<OpenSim::Component> prototype;
        properties_editor::State prop_editor;
        std::vector<OpenSim::AbstractSocket const*> physframe_sockets = get_pf_sockets(*prototype);
        std::vector<OpenSim::PhysicalFrame const*> physframe_connectee_choices =
            std::vector<OpenSim::PhysicalFrame const*>(physframe_sockets.size());

        State(std::unique_ptr<OpenSim::Component> _prototype) : prototype{std::move(_prototype)} {
        }
    };

    // - assumes caller handles ImGui::OpenPopup(modal_name)
    // - returns nullptr until the user fully builds the Component
    std::unique_ptr<OpenSim::Component> draw(State&, char const* modal_name, OpenSim::Model const&);
}

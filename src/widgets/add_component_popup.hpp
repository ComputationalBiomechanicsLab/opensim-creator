#pragma once

#include <memory>

namespace OpenSim {
    class Component;
    class Model;
}

namespace osmv {
    class Add_component_popup final {
    public:
        struct Impl;

    private:
        std::unique_ptr<Impl> impl;

    public:
        Add_component_popup(std::unique_ptr<OpenSim::Component> prototype);
        Add_component_popup(Add_component_popup const&) = default;
        Add_component_popup(Add_component_popup&&) = default;
        Add_component_popup& operator=(Add_component_popup const&) = default;
        Add_component_popup& operator=(Add_component_popup&&) = default;
        ~Add_component_popup() noexcept;

        // - assumes caller handles ImGui::OpenPopup(modal_name)
        // - returns nullptr until the user fully builds the Component
        std::unique_ptr<OpenSim::Component> draw(char const* modal_name, OpenSim::Model const&);
    };
}

#pragma once

#include <memory>

namespace OpenSim
{
    class Component;
    class Model;
}

namespace osc
{
    class AddComponentPopup final {
    public:
        AddComponentPopup(std::unique_ptr<OpenSim::Component> prototype);
        AddComponentPopup(AddComponentPopup const&) = delete;
        AddComponentPopup(AddComponentPopup&&) noexcept;
        ~AddComponentPopup() noexcept;
        AddComponentPopup& operator=(AddComponentPopup const&) = delete;
        AddComponentPopup& operator=(AddComponentPopup&&) noexcept;

        // - assumes caller handles ImGui::OpenPopup(modal_name)
        // - returns nullptr until the user fully builds the Component
        std::unique_ptr<OpenSim::Component> draw(char const* modalName, OpenSim::Model const&);

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}

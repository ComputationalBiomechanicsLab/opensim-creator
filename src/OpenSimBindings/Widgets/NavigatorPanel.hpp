#pragma once

#include "src/Widgets/Panel.hpp"
#include "src/Utils/CStringView.hpp"

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class VirtualModelStatePair; }

namespace osc
{
    class NavigatorPanel final : public Panel {
    public:
        NavigatorPanel(
            std::string_view panelName,
            std::shared_ptr<VirtualModelStatePair>,
            std::function<void(OpenSim::ComponentPath const&)> onRightClick = [](auto const&){}
        );
        NavigatorPanel(NavigatorPanel const&) = delete;
        NavigatorPanel(NavigatorPanel&&) noexcept;
        NavigatorPanel& operator=(NavigatorPanel const&) = delete;
        NavigatorPanel& operator=(NavigatorPanel&&) noexcept;
        ~NavigatorPanel() noexcept;

    private:
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

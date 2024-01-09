#pragma once

#include <oscar/UI/Panels/IPanel.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class IModelStatePair; }

namespace osc
{
    class NavigatorPanel final : public IPanel {
    public:
        NavigatorPanel(
            std::string_view panelName,
            std::shared_ptr<IModelStatePair>,
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
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

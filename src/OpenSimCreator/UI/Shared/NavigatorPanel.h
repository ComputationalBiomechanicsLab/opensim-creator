#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

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
            std::function<void(const OpenSim::ComponentPath&)> onRightClick = [](const auto&){}
        );
        NavigatorPanel(const NavigatorPanel&) = delete;
        NavigatorPanel(NavigatorPanel&&) noexcept;
        NavigatorPanel& operator=(const NavigatorPanel&) = delete;
        NavigatorPanel& operator=(NavigatorPanel&&) noexcept;
        ~NavigatorPanel() noexcept;

    private:
        CStringView impl_get_name() const final;
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

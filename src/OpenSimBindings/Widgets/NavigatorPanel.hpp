#pragma once

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class VirtualModelStatePair; }

namespace osc
{
    class NavigatorPanel final {
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

        bool isOpen() const;
        void open();
        void close();
        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

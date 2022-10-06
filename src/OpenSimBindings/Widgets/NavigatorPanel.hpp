#pragma once

#include <functional>
#include <string_view>

namespace OpenSim { class Component; }
namespace OpenSim { class ComponentPath; }
namespace osc { class VirtualConstModelStatePair; }

namespace osc
{
    class NavigatorPanel final {
    public:
        enum class ResponseType {
            NothingHappened,
            SelectionChanged,
            HoverChanged,
        };

        struct Response final {
            OpenSim::Component const* ptr = nullptr;
            ResponseType type = ResponseType::NothingHappened;
        };

        NavigatorPanel(std::string_view panelName, std::function<void(OpenSim::ComponentPath const&)> onRightClick = [](auto const&){});
        NavigatorPanel(NavigatorPanel const&) = delete;
        NavigatorPanel(NavigatorPanel&&) noexcept;
        NavigatorPanel& operator=(NavigatorPanel const&) = delete;
        NavigatorPanel& operator=(NavigatorPanel&&) noexcept;
        ~NavigatorPanel() noexcept;

        bool isOpen() const;
        void open();
        void close();
        Response draw(VirtualConstModelStatePair const&);

    private:
        class Impl;
        Impl* m_Impl;
    };
}

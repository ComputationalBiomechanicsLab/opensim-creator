#pragma once

#include <oscar/UI/Widgets/Popup.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace osc { class MainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class VirtualModelStatePair; }

namespace osc
{
    class VirtualModelStatePairContextMenu final : public Popup {
    public:
        VirtualModelStatePairContextMenu(
            std::string_view panelName,
            std::shared_ptr<VirtualModelStatePair>,
            ParentPtr<MainUIStateAPI> const&,
            std::optional<std::string> maybeComponentAbsPath
        );
        VirtualModelStatePairContextMenu(VirtualModelStatePairContextMenu const&) = delete;
        VirtualModelStatePairContextMenu(VirtualModelStatePairContextMenu&&) noexcept;
        VirtualModelStatePairContextMenu& operator=(VirtualModelStatePairContextMenu const&) = delete;
        VirtualModelStatePairContextMenu& operator=(VirtualModelStatePairContextMenu&&) noexcept;
        ~VirtualModelStatePairContextMenu() noexcept;

    private:
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        bool implBeginPopup() final;
        void implOnDraw() final;
        void implEndPopup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

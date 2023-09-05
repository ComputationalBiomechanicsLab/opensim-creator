#pragma once

#include <oscar/Widgets/Popup.hpp>

#include <functional>
#include <memory>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class Select1PFPopup final : public Popup {
    public:
        Select1PFPopup(
            std::string_view popupName,
            std::shared_ptr<UndoableModelStatePair const>,
            std::function<void(OpenSim::ComponentPath const&)> onSelection
        );
        Select1PFPopup(Select1PFPopup const&) = delete;
        Select1PFPopup(Select1PFPopup&&) noexcept;
        Select1PFPopup& operator=(Select1PFPopup const&) = delete;
        Select1PFPopup& operator=(Select1PFPopup&&) noexcept;
        ~Select1PFPopup() noexcept;

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

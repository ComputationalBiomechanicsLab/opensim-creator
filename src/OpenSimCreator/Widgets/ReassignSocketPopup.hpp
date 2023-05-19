#pragma once

#include <oscar/Widgets/Popup.hpp>

#include <memory>
#include <string_view>

namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ReassignSocketPopup final : public Popup {
    public:
        ReassignSocketPopup(
            std::string_view popupName,
            std::shared_ptr<UndoableModelStatePair>,
            std::string_view componentAbsPath,
            std::string_view socketName
        );
        ReassignSocketPopup(ReassignSocketPopup const&) = delete;
        ReassignSocketPopup(ReassignSocketPopup&&) noexcept;
        ReassignSocketPopup& operator=(ReassignSocketPopup const&) = delete;
        ReassignSocketPopup& operator=(ReassignSocketPopup&&) noexcept;
        ~ReassignSocketPopup() noexcept;

    private:
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        bool implBeginPopup() final;
        void implDrawPopupContent() final;
        void implEndPopup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

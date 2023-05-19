#pragma once

#include "oscar/Widgets/Popup.hpp"

#include <memory>

namespace osc { struct SaveChangesPopupConfig; }

namespace osc
{
    class SaveChangesPopup final : public Popup {
    public:
        SaveChangesPopup(SaveChangesPopupConfig const&);
        SaveChangesPopup(SaveChangesPopup const&) = delete;
        SaveChangesPopup(SaveChangesPopup&&) noexcept;
        SaveChangesPopup& operator=(SaveChangesPopup const&) = delete;
        SaveChangesPopup& operator=(SaveChangesPopup&&) noexcept;
        ~SaveChangesPopup() noexcept;

        void draw();
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

#pragma once

#include "src/Widgets/VirtualPopup.hpp"

#include <memory>

namespace osc { class SaveChangesPopupConfig; }

namespace osc
{
    class SaveChangesPopup final : public VirtualPopup {
    public:
        SaveChangesPopup(SaveChangesPopupConfig const&);
        SaveChangesPopup(SaveChangesPopup const&) = delete;
        SaveChangesPopup(SaveChangesPopup&&) noexcept = default;
        SaveChangesPopup& operator=(SaveChangesPopup const&) = default;
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

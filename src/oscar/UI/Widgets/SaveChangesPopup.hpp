#pragma once

#include <oscar/UI/Widgets/IPopup.hpp>

#include <memory>

namespace osc { struct SaveChangesPopupConfig; }

namespace osc
{
    class SaveChangesPopup final : public IPopup {
    public:
        explicit SaveChangesPopup(SaveChangesPopupConfig const&);
        SaveChangesPopup(SaveChangesPopup const&) = delete;
        SaveChangesPopup(SaveChangesPopup&&) noexcept;
        SaveChangesPopup& operator=(SaveChangesPopup const&) = delete;
        SaveChangesPopup& operator=(SaveChangesPopup&&) noexcept;
        ~SaveChangesPopup() noexcept;

        void onDraw();
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

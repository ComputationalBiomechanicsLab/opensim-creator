#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <memory>

namespace osc { struct SaveChangesPopupConfig; }

namespace osc
{
    class SaveChangesPopup final : public IPopup {
    public:
        explicit SaveChangesPopup(const SaveChangesPopupConfig&);
        SaveChangesPopup(const SaveChangesPopup&) = delete;
        SaveChangesPopup(SaveChangesPopup&&) noexcept;
        SaveChangesPopup& operator=(const SaveChangesPopup&) = delete;
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

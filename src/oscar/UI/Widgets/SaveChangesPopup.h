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

        void on_draw();
    private:
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        bool impl_begin_popup() final;
        void impl_on_draw() final;
        void impl_end_popup() final;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

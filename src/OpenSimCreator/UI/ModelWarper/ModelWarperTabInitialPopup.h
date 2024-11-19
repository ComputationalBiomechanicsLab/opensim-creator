#pragma once

#include <oscar/UI/Popups/IPopup.h>

#include <memory>
#include <string_view>

namespace osc
{
    class ModelWarperTabInitialPopup final : public IPopup {
    public:
        explicit ModelWarperTabInitialPopup(std::string_view popupName);
        ModelWarperTabInitialPopup(const ModelWarperTabInitialPopup&) = delete;
        ModelWarperTabInitialPopup(ModelWarperTabInitialPopup&&) noexcept;
        ModelWarperTabInitialPopup& operator=(const ModelWarperTabInitialPopup&) = delete;
        ModelWarperTabInitialPopup& operator=(ModelWarperTabInitialPopup&&) noexcept;
        ~ModelWarperTabInitialPopup() noexcept;

    private:
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;

        bool impl_begin_popup() final;
        void impl_on_draw() final;
        void impl_end_popup() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

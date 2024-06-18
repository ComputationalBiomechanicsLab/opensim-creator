#pragma once

#include <oscar/UI/Widgets/IPopup.h>

#include <memory>
#include <string_view>

namespace osc { class IConstModelStatePair; }

namespace osc
{
    class ExportPointsPopup final : public IPopup {
    public:
        ExportPointsPopup(
            std::string_view popupName,
            std::shared_ptr<IConstModelStatePair const>
        );
        ExportPointsPopup(const ExportPointsPopup&) = delete;
        ExportPointsPopup(ExportPointsPopup&&) noexcept;
        ExportPointsPopup& operator=(const ExportPointsPopup&) = delete;
        ExportPointsPopup& operator=(ExportPointsPopup&&) noexcept;
        ~ExportPointsPopup() noexcept;

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

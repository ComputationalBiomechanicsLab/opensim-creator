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
        ExportPointsPopup(ExportPointsPopup const&) = delete;
        ExportPointsPopup(ExportPointsPopup&&) noexcept;
        ExportPointsPopup& operator=(ExportPointsPopup const&) = delete;
        ExportPointsPopup& operator=(ExportPointsPopup&&) noexcept;
        ~ExportPointsPopup() noexcept;

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

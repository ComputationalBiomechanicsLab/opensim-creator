#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc::mow { class UIState; }

namespace osc::mow
{
    class SourceModelViewerPanel final : public IPanel {
    public:
        SourceModelViewerPanel(
            std::string_view panelName_,
            std::shared_ptr<UIState> state_);
        SourceModelViewerPanel(SourceModelViewerPanel const&) = delete;
        SourceModelViewerPanel(SourceModelViewerPanel&&) noexcept;
        SourceModelViewerPanel& operator=(SourceModelViewerPanel const&) = delete;
        SourceModelViewerPanel& operator=(SourceModelViewerPanel&&) noexcept;
        ~SourceModelViewerPanel() noexcept;

    private:
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

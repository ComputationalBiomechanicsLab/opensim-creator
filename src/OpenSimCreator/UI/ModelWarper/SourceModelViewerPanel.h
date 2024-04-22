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
        CStringView impl_get_name() const final;
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

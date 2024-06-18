#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc::mow { class UIState; }

namespace osc::mow
{
    class ResultModelViewerPanel final : public IPanel {
    public:
        ResultModelViewerPanel(
            std::string_view panelName_,
            std::shared_ptr<UIState> state_);
        ResultModelViewerPanel(const ResultModelViewerPanel&) = delete;
        ResultModelViewerPanel(ResultModelViewerPanel&&) noexcept;
        ResultModelViewerPanel& operator=(const ResultModelViewerPanel&) = delete;
        ResultModelViewerPanel& operator=(ResultModelViewerPanel&&) noexcept;
        ~ResultModelViewerPanel() noexcept;

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

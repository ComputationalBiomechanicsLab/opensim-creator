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
        ResultModelViewerPanel(ResultModelViewerPanel const&) = delete;
        ResultModelViewerPanel(ResultModelViewerPanel&&) noexcept;
        ResultModelViewerPanel& operator=(ResultModelViewerPanel const&) = delete;
        ResultModelViewerPanel& operator=(ResultModelViewerPanel&&) noexcept;
        ~ResultModelViewerPanel() noexcept;

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

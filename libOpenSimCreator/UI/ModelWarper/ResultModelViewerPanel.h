#pragma once

#include <libopensimcreator/UI/Shared/ModelViewerPanel.h>

#include <memory>
#include <string_view>

namespace osc::mow { class UIState; }

namespace osc::mow
{
    class ResultModelViewerPanel final : public ModelViewerPanel {
    public:
        ResultModelViewerPanel(std::string_view panelName_, std::shared_ptr<UIState> state_);

    private:
        void impl_draw_content() final;

        std::shared_ptr<UIState> m_State;
    };
}

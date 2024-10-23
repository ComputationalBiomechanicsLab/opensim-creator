#pragma once

#include <OpenSimCreator/UI/Shared/ModelViewerPanel.h>

#include <memory>
#include <string_view>

namespace osc::mow { class UIState; }

namespace osc::mow
{
    class SourceModelViewerPanel final : public ModelViewerPanel {
    public:
        SourceModelViewerPanel(std::string_view panelName_, std::shared_ptr<UIState> state_);

    private:
        void impl_on_draw() final;

        std::shared_ptr<UIState> m_State;
    };
}

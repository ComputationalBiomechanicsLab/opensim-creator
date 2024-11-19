#pragma once

#include <OpenSimCreator/UI/ModelWarper/UIState.h>

#include <oscar/UI/Panels/Panel.h>

#include <memory>
#include <string_view>
#include <utility>

namespace osc::mow
{
    class ChecklistPanel final : public Panel {
    public:
        ChecklistPanel(
            std::string_view panelName_,
            std::shared_ptr<UIState> state_) :

            Panel{nullptr, panelName_},
            m_State{std::move(state_)}
        {}
    private:
        void impl_draw_content() final;

        std::shared_ptr<UIState> m_State;
    };
}

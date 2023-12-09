#pragma once

#include <OpenSimCreator/UI/ModelWarper/UIState.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace osc::mow
{
    class Toolbar final {
    public:
        Toolbar(
            std::string_view label_,
            std::shared_ptr<UIState> state_) :

            m_Label{label_},
            m_State{std::move(state_)}
        {
        }

        void onDraw();
    private:
        void drawContent();

        std::string m_Label;
        std::shared_ptr<UIState> m_State;
    };
}

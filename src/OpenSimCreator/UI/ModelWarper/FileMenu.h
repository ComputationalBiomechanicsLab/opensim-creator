#pragma once

#include <OpenSimCreator/UI/ModelWarper/UIState.h>

#include <memory>
#include <utility>

namespace osc::mow
{
    class FileMenu final {
    public:
        explicit FileMenu(std::shared_ptr<UIState> state_) :
            m_State{std::move(state_)}
        {}

        void onDraw();

    private:
        void drawContent();

        std::shared_ptr<UIState> m_State;
    };
}

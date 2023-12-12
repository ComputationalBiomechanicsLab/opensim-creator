#pragma once

#include <OpenSimCreator/UI/ModelWarper/UIState.hpp>

#include <oscar/UI/Panels/StandardPanel.hpp>

#include <memory>
#include <string_view>
#include <utility>

namespace OpenSim { class Frame; }
namespace OpenSim { class Mesh; }

namespace osc::mow
{
    class ChecklistPanel final : public StandardPanel {
    public:
        ChecklistPanel(
            std::string_view panelName_,
            std::shared_ptr<UIState> state_) :

            StandardPanel{panelName_},
            m_State{std::move(state_)}
        {
        }
    private:
        void implDrawContent() final;

        std::shared_ptr<UIState> m_State;
    };
}

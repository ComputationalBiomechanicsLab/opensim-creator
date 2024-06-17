#pragma once

#include <functional>
#include <memory>
#include <utility>

namespace osc { struct SimulationViewerRightClickEvent; }
namespace osc { class IModelStatePair; }

namespace osc
{
    class SimulationViewerPanelParameters final {
    public:
        SimulationViewerPanelParameters(
            std::shared_ptr<IModelStatePair> model_,
            const std::function<void(const SimulationViewerRightClickEvent&)>& onRightClickedAComponent_) :

            m_Model{std::move(model_)},
            m_OnRightClickedAComponent{onRightClickedAComponent_}
        {
        }

        IModelStatePair& updModelState() { return *m_Model; }
        void callOnRightClickHandler(const SimulationViewerRightClickEvent& e) const { m_OnRightClickedAComponent(e); }

    private:
        std::shared_ptr<IModelStatePair> m_Model;
        std::function<void(const SimulationViewerRightClickEvent&)> m_OnRightClickedAComponent;
    };
}

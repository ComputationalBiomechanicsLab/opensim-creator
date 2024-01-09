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
            std::function<void(SimulationViewerRightClickEvent const&)> const& onRightClickedAComponent_) :

            m_Model{std::move(model_)},
            m_OnRightClickedAComponent{onRightClickedAComponent_}
        {
        }

        IModelStatePair& updModelState() { return *m_Model; }
        void callOnRightClickHandler(SimulationViewerRightClickEvent const& e) const { m_OnRightClickedAComponent(e); }

    private:
        std::shared_ptr<IModelStatePair> m_Model;
        std::function<void(SimulationViewerRightClickEvent const&)> m_OnRightClickedAComponent;
    };
}

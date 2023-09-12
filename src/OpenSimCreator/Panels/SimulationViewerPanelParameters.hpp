#pragma once

#include <functional>
#include <memory>
#include <utility>

namespace osc { struct SimulationViewerRightClickEvent; }
namespace osc { class VirtualModelStatePair; }

namespace osc
{
    class SimulationViewerPanelParameters final {
    public:
        SimulationViewerPanelParameters(
            std::shared_ptr<VirtualModelStatePair> model_,
            std::function<void(SimulationViewerRightClickEvent const&)> const& onRightClickedAComponent_) :

            m_Model{std::move(model_)},
            m_OnRightClickedAComponent{onRightClickedAComponent_}
        {
        }

        VirtualModelStatePair& updModelState() { return *m_Model; }
        void callOnRightClickHandler(SimulationViewerRightClickEvent const& e) const { m_OnRightClickedAComponent(e); }

    private:
        std::shared_ptr<VirtualModelStatePair> m_Model;
        std::function<void(SimulationViewerRightClickEvent const&)> m_OnRightClickedAComponent;
    };
}

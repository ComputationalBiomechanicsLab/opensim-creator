#pragma once

#include <libopynsim/graphics/model_renderer_params.h>

#include <functional>
#include <memory>
#include <utility>

namespace osc { class ModelStatePair; }
namespace osc { struct ModelViewerPanelRightClickEvent; }
namespace osc { struct ModelRendererParams; }

namespace osc
{
    class ModelViewerPanelParameters final {
    public:
        explicit ModelViewerPanelParameters(
            std::shared_ptr<ModelStatePair> model_,
            const std::function<void(const ModelViewerPanelRightClickEvent&)>& onRightClickedAComponent_ = [](const auto&) {}) :

            m_Model{std::move(model_)},
            m_OnRightClickedAComponent{onRightClickedAComponent_}
        {}

        std::shared_ptr<ModelStatePair> getModelSharedPtr() { return m_Model; }
        void setModelSharedPtr(const std::shared_ptr<ModelStatePair>& newModelState) { m_Model = newModelState; }
        void callOnRightClickHandler(const ModelViewerPanelRightClickEvent& e) { m_OnRightClickedAComponent(e); }
        const ModelRendererParams& getRenderParams() const { return m_RenderParams; }
        ModelRendererParams& updRenderParams() { return m_RenderParams; }

    private:
        std::shared_ptr<ModelStatePair> m_Model;
        std::function<void(const ModelViewerPanelRightClickEvent&)> m_OnRightClickedAComponent;
        ModelRendererParams m_RenderParams;
    };
}

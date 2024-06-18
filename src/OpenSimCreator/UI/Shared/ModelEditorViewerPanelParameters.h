#pragma once

#include <OpenSimCreator/Graphics/ModelRendererParams.h>

#include <functional>
#include <memory>
#include <utility>

namespace osc { struct ModelEditorViewerPanelRightClickEvent; }
namespace osc { struct ModelRendererParams; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorViewerPanelParameters final {
    public:
        ModelEditorViewerPanelParameters(
            std::shared_ptr<UndoableModelStatePair> model_,
            const std::function<void(const ModelEditorViewerPanelRightClickEvent&)>& onRightClickedAComponent_) :

            m_Model{std::move(model_)},
            m_OnRightClickedAComponent{onRightClickedAComponent_}
        {}

        std::shared_ptr<UndoableModelStatePair> getModelSharedPtr() { return m_Model; }
        void callOnRightClickHandler(const ModelEditorViewerPanelRightClickEvent& e) { m_OnRightClickedAComponent(e); }
        const ModelRendererParams& getRenderParams() const { return m_RenderParams; }
        ModelRendererParams& updRenderParams() { return m_RenderParams; }

    private:
        std::shared_ptr<UndoableModelStatePair> m_Model;
        std::function<void(const ModelEditorViewerPanelRightClickEvent&)> m_OnRightClickedAComponent;
        ModelRendererParams m_RenderParams;
    };
}

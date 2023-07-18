#pragma once

#include "OpenSimCreator/Graphics/ModelRendererParams.hpp"

#include <functional>
#include <memory>

namespace osc { struct ModelEditorViewerPanelRightClickEvent; }
namespace osc { struct ModelRendererParams; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorViewerPanelParameters final {
    public:
        ModelEditorViewerPanelParameters(
            std::shared_ptr<UndoableModelStatePair> model_,
            std::function<void(ModelEditorViewerPanelRightClickEvent const&)> const& onRightClickedAComponent_) :

            m_Model{std::move(model_)},
            m_OnRightClickedAComponent{onRightClickedAComponent_}
        {
        }

        std::shared_ptr<UndoableModelStatePair> getModelSharedPtr() { return m_Model; }
        void callOnRightClickHandler(ModelEditorViewerPanelRightClickEvent const& e) { m_OnRightClickedAComponent(e); }
        ModelRendererParams const& getRenderParams() const { return m_RenderParams; }
        ModelRendererParams& updRenderParams() { return m_RenderParams; }

    private:
        std::shared_ptr<UndoableModelStatePair> m_Model;
        std::function<void(ModelEditorViewerPanelRightClickEvent const&)> m_OnRightClickedAComponent;
        ModelRendererParams m_RenderParams;
    };
}
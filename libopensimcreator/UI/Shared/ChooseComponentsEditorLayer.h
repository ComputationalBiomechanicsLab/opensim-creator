#pragma once

#include <libopensimcreator/UI/Shared/ChooseComponentsEditorLayerParameters.h>
#include <libopensimcreator/UI/Shared/ModelViewerPanelLayer.h>

#include <memory>

namespace opyn { class ModelStatePair; }
namespace osc { class ModelViewerPanelParameters; }
namespace osc { class ModelViewerPanelState; }

namespace osc
{
    // modal popup that prompts the user to select components in the model (e.g.
    // to define an edge, or a frame)
    class ChooseComponentsEditorLayer final : public ModelViewerPanelLayer {
    public:
        ChooseComponentsEditorLayer(
            std::shared_ptr<opyn::ModelStatePair>,
            ChooseComponentsEditorLayerParameters
        );
        ChooseComponentsEditorLayer(const ChooseComponentsEditorLayer&) = delete;
        ChooseComponentsEditorLayer(ChooseComponentsEditorLayer&&) noexcept;
        ChooseComponentsEditorLayer& operator=(const ChooseComponentsEditorLayer&) = delete;
        ChooseComponentsEditorLayer& operator=(ChooseComponentsEditorLayer&&) noexcept;
        ~ChooseComponentsEditorLayer() noexcept;

    private:
        bool implHandleKeyboardInputs(
            ModelViewerPanelParameters&,
            ModelViewerPanelState&
        ) final;

        bool implHandleMouseInputs(
            ModelViewerPanelParameters&,
            ModelViewerPanelState&
        ) final;

        void implOnDraw(
            ModelViewerPanelParameters&,
            ModelViewerPanelState&
        ) final;

        float implGetBackgroundAlpha() const final;

        bool implShouldClose() const final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

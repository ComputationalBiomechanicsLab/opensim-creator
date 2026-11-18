#pragma once

#include <libopensimcreator/ui/shared/choose_components_editor_layer_parameters.h>
#include <libopensimcreator/ui/shared/model_viewer_panel_layer.h>

#include <memory>

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    class ModelViewerPanelParameters;
    class ModelViewerPanelState;

    // modal popup that prompts the user to select components in the model (e.g.
    // to define an edge, or a frame)
    class ChooseComponentsEditorLayer final : public ModelViewerPanelLayer {
    public:
        ChooseComponentsEditorLayer(
            std::shared_ptr<opyn::ModelStatePair>, ChooseComponentsEditorLayerParameters);
        ChooseComponentsEditorLayer(const ChooseComponentsEditorLayer&) = delete;
        ChooseComponentsEditorLayer(ChooseComponentsEditorLayer&&) noexcept;
        ChooseComponentsEditorLayer& operator=(const ChooseComponentsEditorLayer&) = delete;
        ChooseComponentsEditorLayer& operator=(ChooseComponentsEditorLayer&&) noexcept;
        ~ChooseComponentsEditorLayer() noexcept override;

    private:
        bool impl_handle_keyboard_inputs(
            ModelViewerPanelParameters&, ModelViewerPanelState&) override;

        bool impl_handle_mouse_inputs(ModelViewerPanelParameters&, ModelViewerPanelState&) override;

        void impl_on_draw(ModelViewerPanelParameters&, ModelViewerPanelState&) override;

        float impl_get_background_alpha() const override;

        bool impl_should_close() const override;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

#pragma once

#include <libopensimcreator/ui/shared/model_viewer_panel_layer_flags.h>

namespace osc
{
    class ModelViewerPanelParameters;
    class ModelViewerPanelState;

    class ModelViewerPanelLayer {
    protected:
        ModelViewerPanelLayer() = default;
        ModelViewerPanelLayer(const ModelViewerPanelLayer&) = default;
        ModelViewerPanelLayer(ModelViewerPanelLayer&&) noexcept = default;
        ModelViewerPanelLayer& operator=(const ModelViewerPanelLayer&) = default;
        ModelViewerPanelLayer& operator=(ModelViewerPanelLayer&&) noexcept = default;

    public:
        virtual ~ModelViewerPanelLayer() noexcept = default;

        ModelViewerPanelLayerFlags get_flags() const { return impl_get_flags(); }

        float get_background_alpha() const { return impl_get_background_alpha(); }

        void on_new_frame() { impl_on_new_frame(); }

        bool handle_mouse_inputs(ModelViewerPanelParameters& params, ModelViewerPanelState& state)
        {
            return impl_handle_mouse_inputs(params, state);
        }

        bool handle_keyboard_inputs(
            ModelViewerPanelParameters& params, ModelViewerPanelState& state)
        {
            return impl_handle_keyboard_inputs(params, state);
        }

        void on_draw(ModelViewerPanelParameters& params, ModelViewerPanelState& state)
        {
            impl_on_draw(params, state);
        }

        bool should_close() const { return impl_should_close(); }

    private:
        virtual ModelViewerPanelLayerFlags impl_get_flags() const
        {
            return ModelViewerPanelLayerFlags::Default;
        }
        virtual float impl_get_background_alpha() const { return 0.0f; }
        virtual void impl_on_new_frame() {}
        virtual bool impl_handle_mouse_inputs(ModelViewerPanelParameters&, ModelViewerPanelState&)
        {
            return false;
        }
        virtual bool impl_handle_keyboard_inputs(
            ModelViewerPanelParameters&, ModelViewerPanelState&)
        {
            return false;
        }
        virtual bool impl_should_close() const { return false; }

        virtual void impl_on_draw(ModelViewerPanelParameters&, ModelViewerPanelState&) = 0;
    };
}

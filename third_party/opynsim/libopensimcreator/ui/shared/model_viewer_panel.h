#pragma once

#include <libopensimcreator/ui/shared/model_viewer_panel_flags.h>

#include <liboscar/maths/rect.h>
#include <liboscar/maths/vector.h>
#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <optional>
#include <string_view>

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    class Camera;
    class ModelViewerPanelLayer;
    class ModelViewerPanelParameters;

    class ModelViewerPanel : public Panel {
    public:
        explicit ModelViewerPanel(
            Widget* parent,
            std::string_view panel_name,
            const ModelViewerPanelParameters&,
            ModelViewerPanelFlags = {});

        bool is_moused_over() const;
        bool is_left_clicked() const;
        bool is_right_clicked() const;
        ModelViewerPanelLayer& push_layer(std::unique_ptr<ModelViewerPanelLayer>);
        void focus_on(const Vector3&);
        std::optional<Rect> get_screen_rect() const;
        const Camera& get_camera() const;
        Camera& upd_camera();
        const OrbitCameraController& get_orbit_camera_controller() const;
        OrbitCameraController& upd_orbit_camera_controller();

        void set_model_state(const std::shared_ptr<opyn::ModelStatePair>&);

    protected:
        void impl_draw_content() override;

    private:
        void impl_before_imgui_begin() override;
        void impl_after_imgui_begin() override;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

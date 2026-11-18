#pragma once

#include <libopynsim/graphics/model_renderer_params.h>

#include <liboscar/graphics/orbit_camera_controller.h>

#include <functional>
#include <memory>
#include <utility>

namespace opyn
{
    class ModelStatePair;
}

namespace osc
{
    struct ModelViewerPanelRightClickEvent;

    class ModelViewerPanelParameters final {
    public:
        explicit ModelViewerPanelParameters(
            std::shared_ptr<opyn::ModelStatePair> model,
            const std::function<void(const ModelViewerPanelRightClickEvent&)>&
                on_right_clicked_a_component = [](const auto&) {}) :

            model_{std::move(model)},
            on_right_clicked_a_component_{on_right_clicked_a_component}
        {}

        std::shared_ptr<opyn::ModelStatePair> get_model_shared_ptr() { return model_; }
        void set_model_shared_ptr(const std::shared_ptr<opyn::ModelStatePair>& new_model_state)
        {
            model_ = new_model_state;
        }
        void call_on_right_click_handler(const ModelViewerPanelRightClickEvent& e)
        {
            on_right_clicked_a_component_(e);
        }

        const opyn::ModelRendererParams& get_render_params() const { return render_params_; }
        opyn::ModelRendererParams& upd_render_params() { return render_params_; }

        const OrbitCameraController& get_orbit_camera_controller() const
        {
            return orbit_camera_controller_;
        }
        OrbitCameraController& upd_orbit_camera_controller() { return orbit_camera_controller_; }

        const Camera& get_camera() const { return render_params_.camera; }
        Camera& upd_camera() { return render_params_.camera; }

    private:
        std::shared_ptr<opyn::ModelStatePair> model_;
        std::function<void(const ModelViewerPanelRightClickEvent&)> on_right_clicked_a_component_;
        opyn::ModelRendererParams render_params_;
        OrbitCameraController orbit_camera_controller_;
    };
}

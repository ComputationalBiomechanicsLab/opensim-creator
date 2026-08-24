#pragma once

#include <libopensimcreator/ui/shared/model_viewer_panel_flags.h>

#include <liboscar/maths/rect.h>
#include <liboscar/maths/vector.h>
#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <optional>
#include <string_view>

namespace opyn { class ModelStatePair; }
namespace osc { class Camera; }
namespace osc { class ModelViewerPanelLayer; }
namespace osc { class ModelViewerPanelParameters; }

namespace osc
{
    class ModelViewerPanel : public Panel {
    public:
        explicit ModelViewerPanel(
            Widget* parent_,
            std::string_view panelName_,
            const ModelViewerPanelParameters&,
            ModelViewerPanelFlags = {}
        );

        bool isMousedOver() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;
        ModelViewerPanelLayer& pushLayer(std::unique_ptr<ModelViewerPanelLayer>);
        void focusOn(const Vector3&);
        std::optional<Rect> getScreenRect() const;
        const Camera& getCamera() const;
        Camera& updCamera();
        const OrbitCameraController& getOrbitCameraController() const;
        OrbitCameraController& updOrbitCameraController();

        void setModelState(const std::shared_ptr<opyn::ModelStatePair>&);

    protected:
        void impl_draw_content() override;
    private:
        void impl_before_imgui_begin() final;
        void impl_after_imgui_begin() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

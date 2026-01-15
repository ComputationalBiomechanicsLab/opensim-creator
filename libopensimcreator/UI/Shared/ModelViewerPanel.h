#pragma once

#include <libopensimcreator/UI/Shared/ModelViewerPanelFlags.h>

#include <liboscar/maths/Rect.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/ui/panels/panel.h>

#include <memory>
#include <optional>
#include <string_view>

namespace osc { class IModelStatePair; }
namespace osc { class ModelViewerPanelLayer; }
namespace osc { class ModelViewerPanelParameters; }
namespace osc { struct PolarPerspectiveCamera; }

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
        const PolarPerspectiveCamera& getCamera() const;
        void setCamera(const PolarPerspectiveCamera&);
        void setModelState(const std::shared_ptr<IModelStatePair>&);

    protected:
        void impl_draw_content() override;
    private:
        void impl_before_imgui_begin() final;
        void impl_after_imgui_begin() final;

        class Impl;
        OSC_WIDGET_DATA_GETTERS(Impl);
    };
}

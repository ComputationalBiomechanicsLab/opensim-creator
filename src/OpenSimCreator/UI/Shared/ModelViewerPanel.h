#pragma once

#include <OpenSimCreator/UI/Shared/ModelViewerPanelFlags.h>

#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/UI/Panels/Panel.h>

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
            std::string_view panelName_,
            const ModelViewerPanelParameters&,
            ModelViewerPanelFlags = {}
        );

        bool isMousedOver() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;
        ModelViewerPanelLayer& pushLayer(std::unique_ptr<ModelViewerPanelLayer>);
        void focusOn(const Vec3&);
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

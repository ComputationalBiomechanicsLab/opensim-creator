#pragma once

#include <OpenSimCreator/UI/Shared/ModelViewerPanelFlags.h>

#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/Flags.h>

#include <memory>
#include <optional>
#include <string_view>

namespace osc { class CustomRenderingOptions; }
namespace osc { class ModelViewerPanelLayer; }
namespace osc { class ModelViewerPanelParameters; }
namespace osc { struct PolarPerspectiveCamera; }

namespace osc
{
    class ModelViewerPanel final : public IPanel {
    public:
        ModelViewerPanel(
            std::string_view panelName_,
            const ModelViewerPanelParameters&,
            ModelViewerPanelFlags = {}
        );
        ModelViewerPanel(const ModelViewerPanel&) = delete;
        ModelViewerPanel(ModelViewerPanel&&) noexcept;
        ModelViewerPanel& operator=(const ModelViewerPanel&) = delete;
        ModelViewerPanel& operator=(ModelViewerPanel&&) noexcept;
        ~ModelViewerPanel() noexcept;

        bool isMousedOver() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;
        ModelViewerPanelLayer& pushLayer(std::unique_ptr<ModelViewerPanelLayer>);
        void focusOn(const Vec3&);
        std::optional<Rect> getScreenRect() const;
        const PolarPerspectiveCamera& getCamera() const;
        void setCamera(const PolarPerspectiveCamera&);

    private:
        CStringView impl_get_name() const final;
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

#pragma once

#include <oscar/Maths/Vec3.h>
#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc { struct Color; }
namespace osc { class CustomRenderingOptions; }
namespace osc { class ModelEditorViewerPanelLayer; }
namespace osc { class ModelEditorViewerPanelParameters; }

namespace osc
{
    class ModelEditorViewerPanel final : public IPanel {
    public:
        ModelEditorViewerPanel(
            std::string_view panelName_,
            ModelEditorViewerPanelParameters const&
        );
        ModelEditorViewerPanel(ModelEditorViewerPanel const&) = delete;
        ModelEditorViewerPanel(ModelEditorViewerPanel&&) noexcept;
        ModelEditorViewerPanel& operator=(ModelEditorViewerPanel const&) = delete;
        ModelEditorViewerPanel& operator=(ModelEditorViewerPanel&&) noexcept;
        ~ModelEditorViewerPanel() noexcept;

        ModelEditorViewerPanelLayer& pushLayer(std::unique_ptr<ModelEditorViewerPanelLayer>);
        void focusOn(Vec3 const&);

    private:
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

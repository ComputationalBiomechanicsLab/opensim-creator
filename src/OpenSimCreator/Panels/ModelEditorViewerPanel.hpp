#pragma once

#include <oscar/Panels/Panel.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <glm/vec3.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace OpenSim { class ComponentPath; }
namespace osc { struct Color; }
namespace osc { class CustomRenderingOptions; }
namespace osc { class EditorAPI; }
namespace osc { class MainUIStateAPI; }
namespace osc { struct ModelEditorViewerPanelRightClickEvent; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorViewerPanel final : public Panel {
    public:
        ModelEditorViewerPanel(
            std::string_view panelName_,
            std::weak_ptr<MainUIStateAPI>,
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        ModelEditorViewerPanel(
            std::string_view panelName_,
            std::shared_ptr<UndoableModelStatePair>,
            std::function<void(ModelEditorViewerPanelRightClickEvent const&)> const& onRightClickedAComponent
        );
        ModelEditorViewerPanel(ModelEditorViewerPanel const&) = delete;
        ModelEditorViewerPanel(ModelEditorViewerPanel&&) noexcept;
        ModelEditorViewerPanel& operator=(ModelEditorViewerPanel const&) = delete;
        ModelEditorViewerPanel& operator=(ModelEditorViewerPanel&&) noexcept;
        ~ModelEditorViewerPanel() noexcept;

        CustomRenderingOptions const& getCustomRenderingOptions() const;
        void setCustomRenderingOptions(CustomRenderingOptions const&);
        void setBackgroundColor(Color const&);

    private:
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
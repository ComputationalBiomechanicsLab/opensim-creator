#pragma once

#include <memory>
#include <string_view>

namespace osc { class EditorAPI; }
namespace osc { class MainUIStateAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorViewerPanel final {
    public:
        ModelEditorViewerPanel(
            std::string_view panelName,
            MainUIStateAPI*,
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        ModelEditorViewerPanel(ModelEditorViewerPanel const&) = delete;
        ModelEditorViewerPanel(ModelEditorViewerPanel&&) noexcept;
        ModelEditorViewerPanel& operator=(ModelEditorViewerPanel const&) = delete;
        ModelEditorViewerPanel& operator=(ModelEditorViewerPanel&&) noexcept;
        ~ModelEditorViewerPanel() noexcept;

        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
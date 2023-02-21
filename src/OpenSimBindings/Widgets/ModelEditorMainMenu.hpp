#pragma once

#include <memory>

namespace osc { class EditorAPI; }
namespace osc { class MainUIStateAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorMainMenu final {
    public:
        ModelEditorMainMenu(
            std::weak_ptr<MainUIStateAPI>,
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        ModelEditorMainMenu(ModelEditorMainMenu const&) = delete;
        ModelEditorMainMenu(ModelEditorMainMenu&&) noexcept;
        ModelEditorMainMenu& operator=(ModelEditorMainMenu const&) = delete;
        ModelEditorMainMenu& operator=(ModelEditorMainMenu&&) noexcept;
        ~ModelEditorMainMenu() noexcept;

        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
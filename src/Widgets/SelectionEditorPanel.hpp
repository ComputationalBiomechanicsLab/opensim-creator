#pragma once

#include <memory>

namespace osc { class EditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class SelectionEditorPanel final {
    public:
        explicit SelectionEditorPanel(EditorAPI*, std::shared_ptr<UndoableModelStatePair>);
        SelectionEditorPanel(SelectionEditorPanel const&) = delete;
        SelectionEditorPanel(SelectionEditorPanel&&) noexcept;
        SelectionEditorPanel& operator=(SelectionEditorPanel const&) = delete;
        SelectionEditorPanel& operator=(SelectionEditorPanel&&) noexcept;
        ~SelectionEditorPanel() noexcept;

        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}

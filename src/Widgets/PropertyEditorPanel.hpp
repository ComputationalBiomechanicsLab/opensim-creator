#pragma once

#include <memory>

namespace osc { class EditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class PropertyEditorPanel final {
    public:
        explicit PropertyEditorPanel(EditorAPI*, std::shared_ptr<UndoableModelStatePair>);
        PropertyEditorPanel(PropertyEditorPanel const&) = delete;
        PropertyEditorPanel(PropertyEditorPanel&&) noexcept;
        PropertyEditorPanel& operator=(PropertyEditorPanel const&) = delete;
        PropertyEditorPanel& operator=(PropertyEditorPanel&&) noexcept;
        ~PropertyEditorPanel() noexcept;

        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}

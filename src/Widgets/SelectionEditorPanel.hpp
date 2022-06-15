#pragma once

#include <memory>

namespace osc
{
    class UndoableModelStatePair;
}

namespace osc
{
    struct SelectionEditorPanel final {
    public:
        explicit SelectionEditorPanel(std::shared_ptr<UndoableModelStatePair>);
        SelectionEditorPanel(SelectionEditorPanel const&) = delete;
        SelectionEditorPanel(SelectionEditorPanel&&) noexcept;
        SelectionEditorPanel& operator=(SelectionEditorPanel const&) = delete;
        SelectionEditorPanel& operator=(SelectionEditorPanel&&) noexcept;
        ~SelectionEditorPanel() noexcept;

        void draw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

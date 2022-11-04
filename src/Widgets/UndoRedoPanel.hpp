#pragma once

#include <memory>
#include <string_view>

namespace osc { class UndoRedo; }

namespace osc
{
    // a generic panel that shows undo/redo history
    class UndoRedoPanel final {
    public:
        UndoRedoPanel(std::string_view panelName_, std::shared_ptr<UndoRedo>);
        UndoRedoPanel(UndoRedoPanel const&) = delete;
        UndoRedoPanel(UndoRedoPanel&&) noexcept;
        UndoRedoPanel& operator=(UndoRedoPanel const&) = delete;
        UndoRedoPanel& operator=(UndoRedoPanel&&) noexcept;
        ~UndoRedoPanel() noexcept;

        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}
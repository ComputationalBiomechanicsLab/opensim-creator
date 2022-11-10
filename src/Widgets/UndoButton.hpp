#pragma once

#include <memory>

namespace osc { class UndoRedo; }

namespace osc
{
    // a user-visible button, with a history dropdown menu, that performs an undo operation
    class UndoButton final {
    public:
        UndoButton(std::shared_ptr<UndoRedo>);
        UndoButton(UndoButton const&) = delete;
        UndoButton(UndoButton&&) noexcept;
        UndoButton& operator=(UndoButton const&) = delete;
        UndoButton& operator=(UndoButton&&) noexcept;
        ~UndoButton() noexcept;

        void draw();
    private:
        std::shared_ptr<UndoRedo> m_UndoRedo;
    };
}
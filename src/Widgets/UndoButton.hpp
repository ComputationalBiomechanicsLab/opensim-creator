#pragma once

#include <memory>

namespace osc { class UndoRedo; }

namespace osc
{
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
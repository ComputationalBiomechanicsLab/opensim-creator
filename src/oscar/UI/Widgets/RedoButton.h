#pragma once

#include <memory>

namespace osc { class UndoRedoBase; }

namespace osc
{
    class RedoButton final {
    public:
        explicit RedoButton(std::shared_ptr<UndoRedoBase>);
        RedoButton(RedoButton const&) = delete;
        RedoButton(RedoButton&&) noexcept = default;
        RedoButton& operator=(RedoButton const&) = delete;
        RedoButton& operator=(RedoButton&&) noexcept = default;
        ~RedoButton() noexcept;

        void onDraw();
    private:
        std::shared_ptr<UndoRedoBase> m_UndoRedo;
    };
}

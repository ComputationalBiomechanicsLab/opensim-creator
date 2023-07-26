#pragma once

#include <memory>

namespace osc { class UndoRedo; }

namespace osc
{
    class RedoButton final {
    public:
        RedoButton(std::shared_ptr<UndoRedo>);
        RedoButton(RedoButton const&) = delete;
        RedoButton(RedoButton&&) noexcept = default;
        RedoButton& operator=(RedoButton const&) = delete;
        RedoButton& operator=(RedoButton&&) noexcept = default;
        ~RedoButton() noexcept;

        void onDraw();
    private:
        std::shared_ptr<UndoRedo> m_UndoRedo;
    };
}

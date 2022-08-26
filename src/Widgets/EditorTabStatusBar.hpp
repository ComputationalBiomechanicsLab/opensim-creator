#pragma once

#include <memory>

namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class EditorTabStatusBar final {
    public:
        explicit EditorTabStatusBar(std::shared_ptr<UndoableModelStatePair>);
        EditorTabStatusBar(EditorTabStatusBar const&) = delete;
        EditorTabStatusBar(EditorTabStatusBar&&) noexcept;
        EditorTabStatusBar& operator=(EditorTabStatusBar const&) = delete;
        EditorTabStatusBar& operator=(EditorTabStatusBar&&) noexcept;
        ~EditorTabStatusBar() noexcept;

        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}
#pragma once

#include <memory>

namespace osc { class EditorAPI; }
namespace osc { class MainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class EditorTabStatusBar final {
    public:
        EditorTabStatusBar(
            ParentPtr<MainUIStateAPI> const&,
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        EditorTabStatusBar(EditorTabStatusBar const&) = delete;
        EditorTabStatusBar(EditorTabStatusBar&&) noexcept;
        EditorTabStatusBar& operator=(EditorTabStatusBar const&) = delete;
        EditorTabStatusBar& operator=(EditorTabStatusBar&&) noexcept;
        ~EditorTabStatusBar() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

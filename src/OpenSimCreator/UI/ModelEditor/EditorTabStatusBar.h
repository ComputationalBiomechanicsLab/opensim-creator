#pragma once

#include <memory>

namespace osc { class IEditorAPI; }
namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class EditorTabStatusBar final {
    public:
        EditorTabStatusBar(
            const ParentPtr<IMainUIStateAPI>&,
            IEditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        EditorTabStatusBar(const EditorTabStatusBar&) = delete;
        EditorTabStatusBar(EditorTabStatusBar&&) noexcept;
        EditorTabStatusBar& operator=(const EditorTabStatusBar&) = delete;
        EditorTabStatusBar& operator=(EditorTabStatusBar&&) noexcept;
        ~EditorTabStatusBar() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

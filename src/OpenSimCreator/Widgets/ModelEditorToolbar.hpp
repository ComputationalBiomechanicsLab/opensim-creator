#pragma once

#include <memory>
#include <string_view>

namespace osc { class EditorAPI; }
namespace osc { class MainUIStateAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorToolbar final {
    public:
        ModelEditorToolbar(
            std::string_view,
            std::weak_ptr<MainUIStateAPI>,
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        ModelEditorToolbar(ModelEditorToolbar const&) = delete;
        ModelEditorToolbar(ModelEditorToolbar&&) noexcept;
        ModelEditorToolbar& operator=(ModelEditorToolbar const&) = delete;
        ModelEditorToolbar& operator=(ModelEditorToolbar&&) noexcept;
        ~ModelEditorToolbar() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

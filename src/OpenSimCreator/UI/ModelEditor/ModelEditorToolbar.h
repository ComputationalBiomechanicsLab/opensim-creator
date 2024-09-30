#pragma once

#include <memory>
#include <string_view>

namespace osc { class IEditorAPI; }
namespace osc { class UndoableModelStatePair; }
namespace osc { class Widget; }

namespace osc
{
    class ModelEditorToolbar final {
    public:
        ModelEditorToolbar(
            std::string_view label,
            Widget&,
            IEditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        ModelEditorToolbar(const ModelEditorToolbar&) = delete;
        ModelEditorToolbar(ModelEditorToolbar&&) noexcept;
        ModelEditorToolbar& operator=(const ModelEditorToolbar&) = delete;
        ModelEditorToolbar& operator=(ModelEditorToolbar&&) noexcept;
        ~ModelEditorToolbar() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

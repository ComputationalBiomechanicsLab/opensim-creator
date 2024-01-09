#pragma once

#include <memory>

namespace osc { class IEditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelActionsMenuItems final {
    public:
        ModelActionsMenuItems(
            IEditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        ModelActionsMenuItems(ModelActionsMenuItems const&) = delete;
        ModelActionsMenuItems(ModelActionsMenuItems&&) noexcept;
        ModelActionsMenuItems& operator=(ModelActionsMenuItems const&) = delete;
        ModelActionsMenuItems& operator=(ModelActionsMenuItems&&) noexcept;
        ~ModelActionsMenuItems() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

#pragma once

#include <memory>

namespace osc { class EditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelActionsMenuItems final {
    public:
        ModelActionsMenuItems(EditorAPI*, std::shared_ptr<UndoableModelStatePair>);
        ModelActionsMenuItems(ModelActionsMenuItems const&) = delete;
        ModelActionsMenuItems(ModelActionsMenuItems&&) noexcept;
        ModelActionsMenuItems& operator=(ModelActionsMenuItems const&) = delete;
        ModelActionsMenuItems& operator=(ModelActionsMenuItems&&) noexcept;
        ~ModelActionsMenuItems() noexcept;

        void draw();

    private:
        class Impl;
        Impl* m_Impl;
    };
}

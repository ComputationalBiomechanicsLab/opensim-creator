#pragma once

#include <memory>

namespace osc { class IEditorAPI; }
namespace osc { class IMainUIStateAPI; }
namespace osc { template<typename T> class ParentPtr; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorMainMenu final {
    public:
        ModelEditorMainMenu(
            ParentPtr<IMainUIStateAPI> const&,
            IEditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        ModelEditorMainMenu(const ModelEditorMainMenu&) = delete;
        ModelEditorMainMenu(ModelEditorMainMenu&&) noexcept;
        ModelEditorMainMenu& operator=(const ModelEditorMainMenu&) = delete;
        ModelEditorMainMenu& operator=(ModelEditorMainMenu&&) noexcept;
        ~ModelEditorMainMenu() noexcept;

        void onDraw();

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

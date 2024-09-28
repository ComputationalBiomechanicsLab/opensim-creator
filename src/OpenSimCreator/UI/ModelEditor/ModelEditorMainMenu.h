#pragma once

#include <OpenSimCreator/UI/MainUIScreen.h>

#include <oscar/Utils/ParentPtr.h>

#include <memory>

namespace osc { class IEditorAPI; }
namespace osc { class IModelStatePair; }

namespace osc
{
    class ModelEditorMainMenu final {
    public:
        ModelEditorMainMenu(
            const ParentPtr<MainUIScreen>&,
            IEditorAPI*,
            std::shared_ptr<IModelStatePair>
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

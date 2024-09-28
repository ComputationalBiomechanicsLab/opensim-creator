#pragma once

#include <OpenSimCreator/UI/MainUIScreen.h>

#include <oscar/Utils/ParentPtr.h>

#include <memory>
#include <string_view>

namespace osc { class IEditorAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorToolbar final {
    public:
        ModelEditorToolbar(
            std::string_view label,
            const ParentPtr<MainUIScreen>&,
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

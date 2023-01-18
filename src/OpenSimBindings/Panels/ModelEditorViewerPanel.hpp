#pragma once

#include "src/Panels/Panel.hpp"
#include "src/Utils/CStringView.hpp"

#include <memory>
#include <string_view>

namespace osc { class EditorAPI; }
namespace osc { class MainUIStateAPI; }
namespace osc { class UndoableModelStatePair; }

namespace osc
{
    class ModelEditorViewerPanel final : public Panel {
    public:
        ModelEditorViewerPanel(
            std::string_view panelName,
            MainUIStateAPI*,
            EditorAPI*,
            std::shared_ptr<UndoableModelStatePair>
        );
        ModelEditorViewerPanel(ModelEditorViewerPanel const&) = delete;
        ModelEditorViewerPanel(ModelEditorViewerPanel&&) noexcept;
        ModelEditorViewerPanel& operator=(ModelEditorViewerPanel const&) = delete;
        ModelEditorViewerPanel& operator=(ModelEditorViewerPanel&&) noexcept;
        ~ModelEditorViewerPanel() noexcept;

    private:
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}
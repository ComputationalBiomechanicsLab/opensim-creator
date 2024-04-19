#pragma once

#include <oscar/UI/Panels/IPanel.h>
#include <oscar/Utils/CStringView.h>

#include <memory>
#include <string_view>

namespace osc { class UndoRedoBase; }

namespace osc
{
    // a user-visible panel that lists undo/redo history
    class UndoRedoPanel final : public IPanel {
    public:
        static void DrawContent(UndoRedoBase&);

        UndoRedoPanel(std::string_view panelName, std::shared_ptr<UndoRedoBase>);
        UndoRedoPanel(const UndoRedoPanel&) = delete;
        UndoRedoPanel(UndoRedoPanel&&) noexcept;
        UndoRedoPanel& operator=(const UndoRedoPanel&) = delete;
        UndoRedoPanel& operator=(UndoRedoPanel&&) noexcept;
        ~UndoRedoPanel() noexcept;

    private:
        CStringView implGetName() const final;
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implOnDraw() final;

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

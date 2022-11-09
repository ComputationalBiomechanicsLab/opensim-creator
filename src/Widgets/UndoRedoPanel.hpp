#pragma once

#include "src/Widgets/VirtualPanel.hpp"

#include <memory>
#include <string_view>

namespace osc { class UndoRedo; }

namespace osc
{
    // a generic panel that shows undo/redo history
    class UndoRedoPanel final : public VirtualPanel {
    public:
        UndoRedoPanel(std::string_view panelName_, std::shared_ptr<UndoRedo>);
        UndoRedoPanel(UndoRedoPanel const&) = delete;
        UndoRedoPanel(UndoRedoPanel&&) noexcept;
        UndoRedoPanel& operator=(UndoRedoPanel const&) = delete;
        UndoRedoPanel& operator=(UndoRedoPanel&&) noexcept;
        ~UndoRedoPanel() noexcept;

    private:
        bool implIsOpen() const final;
        void implOpen() final;
        void implClose() final;
        void implDraw() final;

        class Impl;
        Impl* m_Impl;
    };
}
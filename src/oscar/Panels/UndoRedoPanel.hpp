#pragma once

#include "oscar/Panels/Panel.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <memory>
#include <string_view>

namespace osc { class UndoRedo; }

namespace osc
{
    // a user-visible panel that lists undo/redo history
    class UndoRedoPanel final : public Panel {
    public:
        UndoRedoPanel(std::string_view panelName_, std::shared_ptr<UndoRedo>);
        UndoRedoPanel(UndoRedoPanel const&) = delete;
        UndoRedoPanel(UndoRedoPanel&&) noexcept;
        UndoRedoPanel& operator=(UndoRedoPanel const&) = delete;
        UndoRedoPanel& operator=(UndoRedoPanel&&) noexcept;
        ~UndoRedoPanel() noexcept;

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

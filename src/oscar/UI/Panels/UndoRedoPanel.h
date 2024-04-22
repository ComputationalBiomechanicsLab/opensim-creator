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
        static void draw_content(UndoRedoBase&);

        UndoRedoPanel(std::string_view panel_name, std::shared_ptr<UndoRedoBase>);
        UndoRedoPanel(const UndoRedoPanel&) = delete;
        UndoRedoPanel(UndoRedoPanel&&) noexcept;
        UndoRedoPanel& operator=(const UndoRedoPanel&) = delete;
        UndoRedoPanel& operator=(UndoRedoPanel&&) noexcept;
        ~UndoRedoPanel() noexcept;

    private:
        CStringView impl_get_name() const final;
        bool impl_is_open() const final;
        void impl_open() final;
        void impl_close() final;
        void impl_on_draw() final;

        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

#pragma once

#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <oscar/UI/oscimgui.h>

#include <memory>
#include <utility>

namespace osc
{
    // widget: the 'edit' menu (a sub menu of the main menu)
    class MeshWarpingTabEditMenu final {
    public:
        explicit MeshWarpingTabEditMenu(
            std::shared_ptr<MeshWarpingTabSharedState> tabState_) :
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (ui::begin_menu("Edit"))
            {
                drawContent();
                ui::end_menu();
            }
        }

    private:

        void drawContent()
        {
            if (ui::draw_menu_item("Undo", "Ctrl+Z", nullptr, m_State->canUndo()))
            {
                m_State->undo();
            }
            if (ui::draw_menu_item("Redo", "Ctrl+Shift+Z", nullptr, m_State->canRedo()))
            {
                m_State->redo();
            }
            ui::draw_separator();
            if (ui::draw_menu_item("Select All", "Ctrl+A"))
            {
                m_State->selectAll();
            }
            if (ui::draw_menu_item("Deselect", "Escape", nullptr, m_State->hasSelection()))
            {
                m_State->clearSelection();
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

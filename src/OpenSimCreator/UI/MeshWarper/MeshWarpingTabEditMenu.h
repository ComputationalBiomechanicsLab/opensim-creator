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
            if (ui::BeginMenu("Edit"))
            {
                drawContent();
                ui::EndMenu();
            }
        }

    private:

        void drawContent()
        {
            if (ui::MenuItem("Undo", "Ctrl+Z", nullptr, m_State->canUndo()))
            {
                m_State->undo();
            }
            if (ui::MenuItem("Redo", "Ctrl+Shift+Z", nullptr, m_State->canRedo()))
            {
                m_State->redo();
            }
            ImGui::Separator();
            if (ui::MenuItem("Select All", "Ctrl+A"))
            {
                m_State->selectAll();
            }
            if (ui::MenuItem("Deselect", "Escape", nullptr, m_State->hasSelection()))
            {
                m_State->clearSelection();
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

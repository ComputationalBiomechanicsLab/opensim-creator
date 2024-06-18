#pragma once

#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>

#include <IconsFontAwesome5.h>
#include <oscar/Platform/App.h>
#include <oscar/UI/oscimgui.h>

#include <memory>
#include <utility>

using osc::lm::LandmarkCSVFlags;

namespace osc
{
    // widget: the 'file' menu (a sub menu of the main menu)
    class MeshWarpingTabFileMenu final {
    public:
        explicit MeshWarpingTabFileMenu(
            std::shared_ptr<MeshWarpingTabSharedState> tabState_) :
            m_State{std::move(tabState_)}
        {}

        void onDraw()
        {
            if (ui::begin_menu("File"))
            {
                drawContent();
                ui::end_menu();
            }
        }
    private:
        void drawContent()
        {
            if (ui::draw_menu_item(ICON_FA_FILE " New", "Ctrl+N"))
            {
                ActionCreateNewDocument(m_State->updUndoable());
            }

            if (ui::begin_menu(ICON_FA_FILE_IMPORT " Import"))
            {
                drawImportMenuContent();
                ui::end_menu();
            }

            if (ui::begin_menu(ICON_FA_FILE_EXPORT " Export"))
            {
                drawExportMenuContent();
                ui::end_menu();
            }

            if (ui::draw_menu_item(ICON_FA_TIMES " Close", "Ctrl+W"))
            {
                m_State->closeTab();
            }

            if (ui::draw_menu_item(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q"))
            {
                App::upd().request_quit();
            }
        }

        void drawImportMenuContent()
        {
            if (ui::draw_menu_item("Source Mesh"))
            {
                ActionLoadMeshFile(m_State->updUndoable(), TPSDocumentInputIdentifier::Source);
            }
            if (ui::draw_menu_item("Destination Mesh"))
            {
                ActionLoadMeshFile(m_State->updUndoable(), TPSDocumentInputIdentifier::Destination);
            }
            if (ui::draw_menu_item("Source Landmarks from CSV"))
            {
                ActionLoadLandmarksFromCSV(m_State->updUndoable(), TPSDocumentInputIdentifier::Source);
            }
            if (ui::draw_menu_item("Destination Landmarks from CSV"))
            {
                ActionLoadLandmarksFromCSV(m_State->updUndoable(), TPSDocumentInputIdentifier::Destination);
            }
            if (ui::draw_menu_item("Non-Participating Landmarks from CSV"))
            {
                ActionLoadNonParticipatingLandmarksFromCSV(m_State->updUndoable());
            }
        }

        void drawExportMenuContent()
        {
            if (ui::draw_menu_item("Source Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(m_State->getScratch(), TPSDocumentInputIdentifier::Source);
            }
            if (ui::draw_menu_item("Destination Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(m_State->getScratch(), TPSDocumentInputIdentifier::Destination);
            }
            if (ui::draw_menu_item("Landmark Pairs to CSV"))
            {
                ActionSavePairedLandmarksToCSV(m_State->getScratch());
            }
            if (ui::draw_menu_item("Landmark Pairs to CSV (no names)"))
            {
                ActionSavePairedLandmarksToCSV(m_State->getScratch(), LandmarkCSVFlags::NoNames);
            }
            if (ui::draw_menu_item("Non-Participating Landmarks to CSV"))
            {
                ActionSaveNonParticipatingLandmarksToCSV(m_State->getScratch());
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

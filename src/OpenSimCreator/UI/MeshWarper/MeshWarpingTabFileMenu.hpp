#pragma once

#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.hpp>
#include <OpenSimCreator/Documents/MeshWarper/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <oscar/Platform/App.hpp>

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
        {
        }

        void onDraw()
        {
            if (ImGui::BeginMenu("File"))
            {
                drawContent();
                ImGui::EndMenu();
            }
        }
    private:
        void drawContent()
        {
            if (ImGui::MenuItem(ICON_FA_FILE " New", "Ctrl+N"))
            {
                ActionCreateNewDocument(m_State->updUndoable());
            }

            if (ImGui::BeginMenu(ICON_FA_FILE_IMPORT " Import"))
            {
                drawImportMenuContent();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(ICON_FA_FILE_EXPORT " Export"))
            {
                drawExportMenuContent();
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem(ICON_FA_TIMES " Close", "Ctrl+W"))
            {
                m_State->closeTab();
            }

            if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit", "Ctrl+Q"))
            {
                App::upd().requestQuit();
            }
        }

        void drawImportMenuContent()
        {
            if (ImGui::MenuItem("Source Mesh"))
            {
                ActionLoadMeshFile(m_State->updUndoable(), TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Mesh"))
            {
                ActionLoadMeshFile(m_State->updUndoable(), TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Source Landmarks from CSV"))
            {
                ActionLoadLandmarksFromCSV(m_State->updUndoable(), TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Landmarks from CSV"))
            {
                ActionLoadLandmarksFromCSV(m_State->updUndoable(), TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Non-Participating Landmarks from CSV"))
            {
                ActionLoadNonParticipatingLandmarksFromCSV(m_State->updUndoable());
            }
        }

        void drawExportMenuContent()
        {
            if (ImGui::MenuItem("Source Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(m_State->getScratch(), TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Landmarks to CSV"))
            {
                ActionSaveLandmarksToCSV(m_State->getScratch(), TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Landmark Pairs to CSV"))
            {
                ActionSavePairedLandmarksToCSV(m_State->getScratch());
            }
            if (ImGui::MenuItem("Landmark Pairs to CSV (no names)"))
            {
                ActionSavePairedLandmarksToCSV(m_State->getScratch(), LandmarkCSVFlags::NoNames);
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

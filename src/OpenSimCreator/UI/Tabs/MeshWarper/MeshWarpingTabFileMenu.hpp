#pragma once

#include <OpenSimCreator/Documents/MeshWarp/TPSDocumentInputIdentifier.hpp>
#include <OpenSimCreator/Documents/MeshWarp/UndoableTPSDocumentActions.hpp>
#include <OpenSimCreator/UI/Tabs/MeshWarper/MeshWarpingTabSharedState.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <oscar/Platform/App.hpp>

#include <memory>
#include <utility>

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
            if (ImGui::MenuItem(ICON_FA_FILE " New"))
            {
                ActionCreateNewDocument(*m_State->editedDocument);
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

            if (ImGui::MenuItem(ICON_FA_TIMES " Close"))
            {
                m_State->tabHost->closeTab(m_State->tabID);
            }

            if (ImGui::MenuItem(ICON_FA_TIMES_CIRCLE " Quit"))
            {
                App::upd().requestQuit();
            }
        }

        void drawImportMenuContent()
        {
            if (ImGui::MenuItem("Source Mesh"))
            {
                ActionBrowseForNewMesh(*m_State->editedDocument, TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Mesh"))
            {
                ActionBrowseForNewMesh(*m_State->editedDocument, TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Source Landmarks from CSV"))
            {
                ActionLoadLandmarksCSV(*m_State->editedDocument, TPSDocumentInputIdentifier::Source);
            }
            if (ImGui::MenuItem("Destination Landmarks from CSV"))
            {
                ActionLoadLandmarksCSV(*m_State->editedDocument, TPSDocumentInputIdentifier::Destination);
            }
            if (ImGui::MenuItem("Non-Participating Landmarks from CSV"))
            {
                ActionLoadNonParticipatingPointsCSV(*m_State->editedDocument);
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
                ActionSaveLandmarksToPairedCSV(m_State->getScratch());
            }
        }

        std::shared_ptr<MeshWarpingTabSharedState> m_State;
    };
}

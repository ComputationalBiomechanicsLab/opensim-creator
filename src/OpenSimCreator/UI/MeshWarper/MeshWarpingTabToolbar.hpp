#pragma once

#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.hpp>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.hpp>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.hpp>
#include <OpenSimCreator/UI/Shared/BasicWidgets.hpp>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/Widgets/RedoButton.hpp>
#include <oscar/UI/Widgets/UndoButton.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

using osc::lm::LandmarkCSVFlags;

namespace osc
{
    // the top toolbar (contains icons for new, save, open, undo, redo, etc.)
    class MeshWarpingTabToolbar final {
    public:
        MeshWarpingTabToolbar(
            std::string_view label,
            std::shared_ptr<MeshWarpingTabSharedState> tabState_) :

            m_Label{label},
            m_State{std::move(tabState_)}
        {
        }

        void onDraw()
        {
            if (osc::BeginToolbar(m_Label))
            {
                drawContent();
            }
            ImGui::End();
        }

    private:
        void drawContent()
        {
            // document-related stuff
            drawNewDocumentButton();
            ImGui::SameLine();
            drawOpenDocumentButton();
            ImGui::SameLine();
            drawSaveLandmarksButton();
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();

            // undo/redo-related stuff
            m_UndoButton.onDraw();
            ImGui::SameLine();
            m_RedoButton.onDraw();
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();

            // camera stuff
            drawCameraLockCheckbox();
            ImGui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
        }

        void drawNewDocumentButton()
        {
            if (ImGui::Button(ICON_FA_FILE))
            {
                ActionCreateNewDocument(m_State->updUndoable());
            }
            DrawTooltipIfItemHovered(
                "Create New Document",
                "Creates the default scene (undoable)"
            );
        }

        void drawOpenDocumentButton()
        {
            ImGui::Button(ICON_FA_FOLDER_OPEN);
            if (ImGui::BeginPopupContextItem("##OpenFolder", ImGuiPopupFlags_MouseButtonLeft))
            {
                if (ImGui::MenuItem("Load Source Mesh"))
                {
                    ActionLoadMeshFile(m_State->updUndoable(), TPSDocumentInputIdentifier::Source);
                }
                if (ImGui::MenuItem("Load Destination Mesh"))
                {
                    ActionLoadMeshFile(m_State->updUndoable(), TPSDocumentInputIdentifier::Destination);
                }
                ImGui::EndPopup();
            }
            DrawTooltipIfItemHovered(
                "Open File",
                "Open Source/Destination data"
            );
        }

        void drawSaveLandmarksButton()
        {
            if (ImGui::Button(ICON_FA_SAVE))
            {
                ActionSavePairedLandmarksToCSV(m_State->getScratch(), LandmarkCSVFlags::NoNames);
            }
            DrawTooltipIfItemHovered(
                "Save Landmarks to CSV (no names)",
                "Saves all pair-able landmarks to a CSV file, for external processing\n\n(legacy behavior: does not export names: use 'File' menu if you want the names)"
            );
        }

        void drawCameraLockCheckbox()
        {
            ImGui::Checkbox("link cameras", &m_State->linkCameras);
            ImGui::SameLine();
            if (!m_State->linkCameras)
            {
                ImGui::BeginDisabled();
            }
            ImGui::Checkbox("only link rotation", &m_State->onlyLinkRotation);
            if (!m_State->linkCameras)
            {
                ImGui::EndDisabled();
            }
        }

        std::string m_Label;
        std::shared_ptr<MeshWarpingTabSharedState> m_State;
        UndoButton m_UndoButton{m_State->editedDocument};
        RedoButton m_RedoButton{m_State->editedDocument};
    };
}

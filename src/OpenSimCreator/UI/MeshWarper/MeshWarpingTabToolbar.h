#pragma once

#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>

#include <IconsFontAwesome5.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/oscimgui_internal.h>
#include <oscar/UI/Widgets/RedoButton.h>
#include <oscar/UI/Widgets/UndoButton.h>

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
            if (BeginToolbar(m_Label))
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
            ui::SameLine();
            drawOpenDocumentButton();
            ui::SameLine();
            drawSaveLandmarksButton();
            ui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ui::SameLine();

            // undo/redo-related stuff
            m_UndoButton.onDraw();
            ui::SameLine();
            m_RedoButton.onDraw();
            ui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ui::SameLine();

            // camera stuff
            drawCameraLockCheckbox();
            ui::SameLine();

            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ui::SameLine();
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
                if (ui::MenuItem("Load Source Mesh"))
                {
                    ActionLoadMeshFile(m_State->updUndoable(), TPSDocumentInputIdentifier::Source);
                }
                if (ui::MenuItem("Load Destination Mesh"))
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
            ui::Checkbox("link cameras", &m_State->linkCameras);
            ui::SameLine();
            if (!m_State->linkCameras)
            {
                ImGui::BeginDisabled();
            }
            ui::Checkbox("only link rotation", &m_State->onlyLinkRotation);
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

#include "MeshWarpingTabToolbar.h"

#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>

#include <IconsFontAwesome5.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/Widgets/RedoButton.h>
#include <oscar/UI/Widgets/UndoButton.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

class osc::MeshWarpingTabToolbar::Impl final {
public:
    Impl(
        std::string_view label,
        std::shared_ptr<MeshWarpingTabSharedState> tabState_) :

        m_Label{label},
        m_State{std::move(tabState_)}
    {}

    void onDraw()
    {
        if (BeginToolbar(m_Label)) {
            drawContent();
        }
        ui::end_panel();
    }

private:
    void drawContent()
    {
        // document-related stuff
        drawNewDocumentButton();
        ui::same_line();
        drawOpenDocumentButton();
        ui::same_line();
        drawSaveLandmarksButton();
        ui::same_line();

        ui::draw_separator(ImGuiSeparatorFlags_Vertical);
        ui::same_line();

        // undo/redo-related stuff
        m_UndoButton.on_draw();
        ui::same_line();
        m_RedoButton.onDraw();
        ui::same_line();

        ui::draw_separator(ImGuiSeparatorFlags_Vertical);
        ui::same_line();

        // camera stuff
        drawCameraLockCheckbox();
        ui::same_line();

        ui::draw_separator(ImGuiSeparatorFlags_Vertical);
        ui::same_line();
    }

    void drawNewDocumentButton()
    {
        if (ui::draw_button(ICON_FA_FILE)) {
            ActionCreateNewDocument(m_State->updUndoable());
        }
        ui::draw_tooltip_if_item_hovered(
            "Create New Document",
            "Creates the default scene (undoable)"
        );
    }

    void drawOpenDocumentButton()
    {
        ui::draw_button(ICON_FA_FOLDER_OPEN);
        if (ui::begin_popup_context_menu("##OpenFolder", ImGuiPopupFlags_MouseButtonLeft)) {
            if (ui::draw_menu_item("Load Source Mesh")) {
                ActionLoadMeshFile(m_State->updUndoable(), TPSDocumentInputIdentifier::Source);
            }
            if (ui::draw_menu_item("Load Destination Mesh")) {
                ActionLoadMeshFile(m_State->updUndoable(), TPSDocumentInputIdentifier::Destination);
            }
            ui::end_popup();
        }
        ui::draw_tooltip_if_item_hovered(
            "Open File",
            "Open Source/Destination data"
        );
    }

    void drawSaveLandmarksButton()
    {
        if (ui::draw_button(ICON_FA_SAVE)) {
            ActionSavePairedLandmarksToCSV(m_State->getScratch(), lm::LandmarkCSVFlags::NoNames);
        }
        ui::draw_tooltip_if_item_hovered(
            "Save Landmarks to CSV (no names)",
            "Saves all pair-able landmarks to a CSV file, for external processing\n\n(legacy behavior: does not export names: use 'File' menu if you want the names)"
        );
    }

    void drawCameraLockCheckbox()
    {
        ui::draw_checkbox("link cameras", &m_State->linkCameras);
        ui::same_line();
        if (not m_State->linkCameras) {
            ui::begin_disabled();
        }
        ui::draw_checkbox("only link rotation", &m_State->onlyLinkRotation);
        if (not m_State->linkCameras) {
            ui::end_disabled();
        }
    }

    std::string m_Label;
    std::shared_ptr<MeshWarpingTabSharedState> m_State;
    UndoButton m_UndoButton{m_State->editedDocument};
    RedoButton m_RedoButton{m_State->editedDocument};
};

osc::MeshWarpingTabToolbar::MeshWarpingTabToolbar(
    std::string_view label,
    std::shared_ptr<MeshWarpingTabSharedState> sharedState) :
    m_Impl{std::make_unique<Impl>(label, std::move(sharedState))}
{}
osc::MeshWarpingTabToolbar::MeshWarpingTabToolbar(MeshWarpingTabToolbar&&) noexcept = default;
osc::MeshWarpingTabToolbar& osc::MeshWarpingTabToolbar::operator=(MeshWarpingTabToolbar&&) noexcept = default;
osc::MeshWarpingTabToolbar::~MeshWarpingTabToolbar() noexcept = default;

void osc::MeshWarpingTabToolbar::onDraw()
{
    m_Impl->onDraw();
}

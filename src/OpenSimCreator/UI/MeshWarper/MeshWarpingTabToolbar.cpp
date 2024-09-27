#include "MeshWarpingTabToolbar.h"

#include <OpenSimCreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <OpenSimCreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <OpenSimCreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>

#include <oscar/Platform/IconCodepoints.h>
#include <oscar/UI/oscimgui.h>
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

        ui::draw_vertical_separator();
        ui::same_line();

        // undo/redo-related stuff
        m_UndoButton.on_draw();
        ui::same_line();
        m_RedoButton.on_draw();
        ui::same_line();

        ui::draw_vertical_separator();
        ui::same_line();

        // camera stuff
        drawCameraLockCheckbox();
        ui::same_line();

        ui::draw_vertical_separator();
        ui::same_line();

        drawVisualAidsMenuButton();
        ui::same_line();
    }

    void drawNewDocumentButton()
    {
        if (ui::draw_button(OSC_ICON_FILE)) {
            ActionCreateNewDocument(m_State->updUndoable());
        }
        ui::draw_tooltip_if_item_hovered(
            "Create New Document",
            "Creates the default scene (undoable)"
        );
    }

    void drawOpenDocumentButton()
    {
        ui::draw_button(OSC_ICON_FOLDER_OPEN);
        if (ui::begin_popup_context_menu("##OpenFolder", ui::PopupFlag::MouseButtonLeft)) {
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
        if (ui::draw_button(OSC_ICON_SAVE)) {
            ActionSavePairedLandmarksToCSV(m_State->getScratch(), lm::LandmarkCSVFlags::NoNames);
        }
        ui::draw_tooltip_if_item_hovered(
            "Save Landmarks to CSV (no names)",
            "Saves all pair-able landmarks to a CSV file, for external processing\n\n(legacy behavior: does not export names: use 'File' menu if you want the names)"
        );
    }

    void drawCameraLockCheckbox()
    {
        {
            bool linked = m_State->isCamerasLinked();
            if (ui::draw_checkbox("link cameras", &linked)) {
                m_State->setCamerasLinked(linked);
            }
        }

        ui::same_line();

        const bool disabled = not m_State->isCamerasLinked();
        if (disabled) {
            ui::begin_disabled();
        }
        bool linkRotation = m_State->isOnlyCameraRotationLinked();
        if (ui::draw_checkbox("only link rotation", &linkRotation)) {
            m_State->setOnlyCameraRotationLinked(linkRotation);
        }
        if (disabled) {
            ui::end_disabled();
        }
    }

    void drawVisualAidsMenuButton()
    {
        if (ui::draw_button("visualization options " OSC_ICON_COG)) {
            ui::open_popup("visualization_options_popup");
        }
        if (ui::begin_popup("visualization_options_popup", {ui::WindowFlag::AlwaysAutoResize, ui::WindowFlag::NoTitleBar, ui::WindowFlag::NoSavedSettings})) {
            DrawRenderingOptionsEditor(m_State->updCustomRenderingOptions());
            DrawOverlayOptionsEditor(m_State->updOverlayDecorationOptions());
            {
                bool wireframe = m_State->isWireframeModeEnabled();
                if (ui::draw_checkbox("Wireframe", &wireframe)) {
                    m_State->setWireframeModeEnabled(wireframe);
                }
            }
            ui::end_popup();
        }
    }

    std::string m_Label;
    std::shared_ptr<MeshWarpingTabSharedState> m_State;
    UndoButton m_UndoButton{m_State->getUndoableSharedPtr()};
    RedoButton m_RedoButton{m_State->getUndoableSharedPtr()};
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

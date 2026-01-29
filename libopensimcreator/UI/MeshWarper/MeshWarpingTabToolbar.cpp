#include "MeshWarpingTabToolbar.h"

#include <libopensimcreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <libopensimcreator/Platform/msmicons.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>

#include <libopynsim/documents/landmarks/named_landmark.h>
#include <liboscar/platform/widget.h>
#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/oscimgui.h>
#include <liboscar/ui/widgets/redo_button.h>
#include <liboscar/ui/widgets/undo_button.h>

#include <memory>
#include <string_view>
#include <utility>

class osc::MeshWarpingTabToolbar::Impl final : public WidgetPrivate {
public:
    explicit Impl(
        Widget& owner,
        Widget* parent,
        std::string_view label,
        std::shared_ptr<MeshWarpingTabSharedState> tabState_) :

        WidgetPrivate{owner, parent},
        m_State{std::move(tabState_)}
    {
        set_name(label);
    }

    void onDraw()
    {
        if (BeginToolbar(name())) {
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

        drawSwapSourceDestinationButton();
        ui::same_line();
    }

    void drawNewDocumentButton()
    {
        if (ui::draw_button(MSMICONS_FILE)) {
            ActionCreateNewDocument(m_State->updUndoable());
        }
        ui::draw_tooltip_if_item_hovered(
            "Create New Document",
            "Creates the default scene (undoable)"
        );
    }

    void drawOpenDocumentButton()
    {
        ui::draw_button(MSMICONS_FOLDER_OPEN);
        if (ui::begin_popup_context_menu("##OpenFolder", ui::PopupFlag::MouseButtonLeft)) {
            if (ui::draw_menu_item("Load Source Mesh")) {
                ActionPromptUserToLoadMeshFile(m_State->getUndoableSharedPtr(), TPSDocumentInputIdentifier::Source);
            }
            if (ui::draw_menu_item("Load Destination Mesh")) {
                ActionPromptUserToLoadMeshFile(m_State->getUndoableSharedPtr(), TPSDocumentInputIdentifier::Destination);
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
        if (ui::draw_button(MSMICONS_SAVE)) {
            ActionPromptUserToSavePairedLandmarksToCSV(m_State->getScratch(), opyn::LandmarkCSVFlags::NoNames);
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
        if (ui::draw_button("visualization options " MSMICONS_COG)) {
            ui::open_popup("visualization_options_popup");
        }
        if (ui::begin_popup("visualization_options_popup", {ui::PanelFlag::AlwaysAutoResize, ui::PanelFlag::NoTitleBar, ui::PanelFlag::NoSavedSettings})) {
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

    void drawSwapSourceDestinationButton()
    {
        if (ui::draw_button("swap source <-> destination")) {
            ActionSwapSourceDestination(m_State->updUndoable());
        }
        ui::draw_tooltip_if_item_hovered("Swap Source <-> Destination", "Swaps the source mesh with the destination mesh.\n\nNote: non-participating landmarks will be left in the source mesh, because they must always be there.");
    }

    std::shared_ptr<MeshWarpingTabSharedState> m_State;
    UndoButton m_UndoButton{&owner(), m_State->getUndoableSharedPtr(), MSMICONS_UNDO};
    RedoButton m_RedoButton{&owner(), m_State->getUndoableSharedPtr(), MSMICONS_REDO};
};

osc::MeshWarpingTabToolbar::MeshWarpingTabToolbar(
    Widget* parent,
    std::string_view label,
    std::shared_ptr<MeshWarpingTabSharedState> sharedState) :
    Widget{std::make_unique<Impl>(*this, parent, label, std::move(sharedState))}
{}
void osc::MeshWarpingTabToolbar::impl_on_draw() { private_data().onDraw(); }

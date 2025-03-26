#include "MeshWarpingTabToolbar.h"

#include <libopensimcreator/Documents/Landmarks/LandmarkCSVFlags.h>
#include <libopensimcreator/Documents/MeshWarper/UndoableTPSDocumentActions.h>
#include <libopensimcreator/UI/MeshWarper/MeshWarpingTabSharedState.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>

#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Platform/Widget.h>
#include <liboscar/Platform/WidgetPrivate.h>
#include <liboscar/UI/oscimgui.h>
#include <liboscar/UI/Widgets/RedoButton.h>
#include <liboscar/UI/Widgets/UndoButton.h>

#include <memory>
#include <string>
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
                ActionLoadMeshFile(m_State->getUndoableSharedPtr(), TPSDocumentInputIdentifier::Source);
            }
            if (ui::draw_menu_item("Load Destination Mesh")) {
                ActionLoadMeshFile(m_State->getUndoableSharedPtr(), TPSDocumentInputIdentifier::Destination);
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

    std::shared_ptr<MeshWarpingTabSharedState> m_State;
    UndoButton m_UndoButton{&owner(), m_State->getUndoableSharedPtr()};
    RedoButton m_RedoButton{&owner(), m_State->getUndoableSharedPtr()};
};

osc::MeshWarpingTabToolbar::MeshWarpingTabToolbar(
    Widget* parent,
    std::string_view label,
    std::shared_ptr<MeshWarpingTabSharedState> sharedState) :
    Widget{std::make_unique<Impl>(*this, parent, label, std::move(sharedState))}
{}
void osc::MeshWarpingTabToolbar::impl_on_draw() { private_data().onDraw(); }

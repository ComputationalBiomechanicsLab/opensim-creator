#include "FrameDefinitionTabToolbar.h"

#include <libopensimcreator/Documents/Model/UndoableModelStatePair.h>
#include <libopensimcreator/Platform/IconCodepoints.h>
#include <libopensimcreator/UI/FrameDefinition/FrameDefinitionUIHelpers.h>
#include <libopensimcreator/UI/Shared/BasicWidgets.h>
#include <libopensimcreator/Utils/OpenSimHelpers.h>

#include <liboscar/Maths/Vec2.h>
#include <liboscar/UI/oscimgui.h>
#include <OpenSim/Simulation/Model/Model.h>

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

osc::FrameDefinitionTabToolbar::FrameDefinitionTabToolbar(
    Widget* parent,
    std::string_view name,
    std::shared_ptr<UndoableModelStatePair> model_) :

    Widget{parent},
    m_Model{std::move(model_)}
{
    set_name(name);
}

void osc::FrameDefinitionTabToolbar::impl_on_draw()
{
    if (BeginToolbar(name(), Vec2{5.0f, 5.0f})) {
        drawContent();
    }
    ui::end_panel();
}

void osc::FrameDefinitionTabToolbar::drawContent()
{
    DrawUndoAndRedoButtons(*m_Model);
    ui::draw_same_line_with_vertical_separator();
    DrawSceneScaleFactorEditorControls(*m_Model);
    ui::draw_same_line_with_vertical_separator();
    drawExportToOpenSimButton();
}

void osc::FrameDefinitionTabToolbar::drawExportToOpenSimButton()
{
    const size_t numBodies = GetNumChildren(m_Model->getModel().getBodySet());

    if (numBodies == 0) {
        ui::begin_disabled();
    }
    if (ui::draw_button(OSC_ICON_FILE_EXPORT " Export to OpenSim")) {
        if (parent()) {
            fd::ActionExportFrameDefinitionSceneModelToEditorTab(*parent(), *m_Model);
        }
    }
    if (numBodies == 0) {
        ui::end_disabled();
    }
    if (ui::is_item_hovered(ui::HoveredFlag::AllowWhenDisabled)) {
        drawExportToOpenSimTooltipContent(numBodies);
    }
}

void osc::FrameDefinitionTabToolbar::drawExportToOpenSimTooltipContent(size_t numBodies)
{
    ui::begin_tooltip();
    ui::draw_tooltip_header_text("Export to OpenSim");
    ui::draw_tooltip_description_spacer();
    ui::draw_tooltip_description_text("Exports the frame definition scene to opensim.");
    if (numBodies == 0) {
        ui::draw_separator();
        ui::draw_text_warning("Warning:");
        ui::same_line();
        ui::draw_text("You currently have %zu bodies defined. Use the 'Add > Body from This' feature on a frame in your scene to add a new body", numBodies);
    }
    ui::end_tooltip();
}

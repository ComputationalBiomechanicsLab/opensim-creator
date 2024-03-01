#include "FrameDefinitionTabToolbar.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/FrameDefinition/FrameDefinitionUIHelpers.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <IconsFontAwesome5.h>
#include <OpenSim/Simulation/Model/Model.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/Utils/ParentPtr.h>

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

osc::FrameDefinitionTabToolbar::FrameDefinitionTabToolbar(
    std::string_view label_,
    ParentPtr<ITabHost> tabHost_,
    std::shared_ptr<UndoableModelStatePair> model_) :

    m_Label{label_},
    m_TabHost{std::move(tabHost_)},
    m_Model{std::move(model_)}
{
}

void osc::FrameDefinitionTabToolbar::onDraw()
{
    if (BeginToolbar(m_Label, Vec2{5.0f, 5.0f}))
    {
        drawContent();
    }
    ui::End();
}

void osc::FrameDefinitionTabToolbar::drawContent()
{
    DrawUndoAndRedoButtons(*m_Model);
    ui::SameLineWithVerticalSeperator();
    DrawSceneScaleFactorEditorControls(*m_Model);
    ui::SameLineWithVerticalSeperator();
    drawExportToOpenSimButton();
}

void osc::FrameDefinitionTabToolbar::drawExportToOpenSimButton()
{
    size_t const numBodies = GetNumChildren(m_Model->getModel().getBodySet());

    if (numBodies == 0)
    {
        ui::BeginDisabled();
    }
    if (ui::Button(ICON_FA_FILE_EXPORT " Export to OpenSim"))
    {
        fd::ActionExportFrameDefinitionSceneModelToEditorTab(m_TabHost, *m_Model);
    }
    if (numBodies == 0)
    {
        ui::EndDisabled();
    }
    if (ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
    {
        drawExportToOpenSimTooltipContent(numBodies);
    }
}

void osc::FrameDefinitionTabToolbar::drawExportToOpenSimTooltipContent(size_t numBodies)
{
    ui::BeginTooltip();
    ui::TooltipHeaderText("Export to OpenSim");
    ui::TooltipDescriptionSpacer();
    ui::TooltipDescriptionText("Exports the frame definition scene to opensim.");
    if (numBodies == 0)
    {
        ui::Separator();
        ui::TextWarning("Warning:");
        ui::SameLine();
        ui::Text("You currently have %zu bodies defined. Use the 'Add > Body from This' feature on a frame in your scene to add a new body", numBodies);
    }
    ui::EndTooltip();
}

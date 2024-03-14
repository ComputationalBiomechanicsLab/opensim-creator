#include "Toolbar.h"

#include <OpenSimCreator/UI/ModelWarper/ModelWarperUIHelpers.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/Color.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>

#include <utility>

void osc::mow::Toolbar::onDraw()
{
    if (BeginToolbar(m_Label)) {
        drawContent();
    }
    ui::End();
}

void osc::mow::Toolbar::drawContent()
{
    DrawOpenModelButtonWithRecentFilesDropdown([this](auto maybeSelection)
    {
        m_State->actionOpenOsimOrPromptUser(std::move(maybeSelection));
    });

    ui::SameLine();

    drawWarpModelButton();
}

void osc::mow::Toolbar::drawWarpModelButton()
{
    if (!m_State->canWarpModel()) {
        ui::BeginDisabled();
    }
    ui::PushStyleColor(ImGuiCol_Button, Color::dark_green());
    if (ui::Button(ICON_FA_PLAY " Warp Model")) {
        m_State->actionWarpModelAndOpenInModelEditor();
    }
    ui::PopStyleColor();
    if (!m_State->canWarpModel()) {
        ui::EndDisabled();
    }

    if (ui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ui::BeginTooltip();
        ui::TooltipHeaderText("Warp Model");
        ui::TooltipDescriptionSpacer();
        ui::TooltipDescriptionText("Warp the model and open the warped model in the model editor");
        if (!m_State->canWarpModel()) {
            ui::TooltipDescriptionSpacer();
            ui::PushStyleColor(ImGuiCol_Text, Color::muted_red());
            ui::Text("Cannot warp the model right now: there are errors that need to be fixed. See the checklist panel.");
            ui::PopStyleColor();
        }
        ui::EndTooltip();
    }

    ui::SameLine();
    ui::SetNextItemWidth(ui::CalcTextSize("should be roughly this long incl label").x);
    float blend = m_State->getWarpBlendingFactor();
    if (ui::SliderFloat("blending", &blend, 0.0f, 1.0f)) {
        m_State->setWarpBlendingFactor(blend);
    }

    ui::SameLine();
    {
        bool v = m_State->isCameraLinked();
        if (ui::Checkbox("link cameras", &v)) {
            m_State->setCameraLinked(v);
        }
    }

    ui::SameLine();
    {
        bool v = m_State->isOnlyCameraRotationLinked();
        if (ui::Checkbox("only link rotation", &v)) {
            m_State->setOnlyCameraRotationLinked(v);
        }
    }
}

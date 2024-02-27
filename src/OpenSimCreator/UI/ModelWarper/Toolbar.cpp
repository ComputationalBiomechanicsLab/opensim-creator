#include "Toolbar.h"

#include <OpenSimCreator/UI/ModelWarper/ModelWarperUIHelpers.h>
#include <OpenSimCreator/UI/Shared/BasicWidgets.h>

#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <oscar/Graphics/Color.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>

#include <utility>

void osc::mow::Toolbar::onDraw()
{
    if (BeginToolbar(m_Label)) {
        drawContent();
    }
    ImGui::End();
}

void osc::mow::Toolbar::drawContent()
{
    DrawOpenModelButtonWithRecentFilesDropdown([this](auto maybeSelection)
    {
        m_State->actionOpenOsimOrPromptUser(std::move(maybeSelection));
    });

    ImGui::SameLine();

    drawWarpModelButton();
}

void osc::mow::Toolbar::drawWarpModelButton()
{
    if (!m_State->canWarpModel()) {
        ImGui::BeginDisabled();
    }
    PushStyleColor(ImGuiCol_Button, Color::darkGreen());
    if (ImGui::Button(ICON_FA_PLAY " Warp Model")) {
        m_State->actionWarpModelAndOpenInModelEditor();
    }
    PopStyleColor();
    if (!m_State->canWarpModel()) {
        ImGui::EndDisabled();
    }

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        BeginTooltip();
        TooltipHeaderText("Warp Model");
        TooltipDescriptionSpacer();
        TooltipDescriptionText("Warp the model and open the warped model in the model editor");
        if (!m_State->canWarpModel()) {
            TooltipDescriptionSpacer();
            PushStyleColor(ImGuiCol_Text, Color::mutedRed());
            ImGui::Text("Cannot warp the model right now: there are errors that need to be fixed. See the checklist panel.");
            PopStyleColor();
        }
        EndTooltip();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::CalcTextSize("should be roughly this long incl label").x);
    float blend = m_State->getWarpBlendingFactor();
    if (ImGui::SliderFloat("blending", &blend, 0.0f, 1.0f)) {
        m_State->setWarpBlendingFactor(blend);
    }
}

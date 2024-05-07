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
    ui::end_panel();
}

void osc::mow::Toolbar::drawContent()
{
    DrawOpenModelButtonWithRecentFilesDropdown([this](auto maybeSelection)
    {
        m_State->actionOpenOsimOrPromptUser(std::move(maybeSelection));
    });

    ui::same_line();

    drawWarpModelButton();
}

void osc::mow::Toolbar::drawWarpModelButton()
{
    if (!m_State->canWarpModel()) {
        ui::begin_disabled();
    }
    ui::push_style_color(ImGuiCol_Button, Color::dark_green());
    if (ui::draw_button(ICON_FA_PLAY " Warp Model")) {
        m_State->actionWarpModelAndOpenInModelEditor();
    }
    ui::pop_style_color();
    if (!m_State->canWarpModel()) {
        ui::end_disabled();
    }

    if (ui::is_item_hovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ui::begin_tooltip();
        ui::draw_tooltip_header_text("Warp Model");
        ui::draw_tooltip_description_spacer();
        ui::draw_tooltip_description_text("Warp the model and open the warped model in the model editor");
        if (!m_State->canWarpModel()) {
            ui::draw_tooltip_description_spacer();
            ui::push_style_color(ImGuiCol_Text, Color::muted_red());
            ui::draw_text("Cannot warp the model right now: there are errors that need to be fixed. See the checklist panel.");
            ui::pop_style_color();
        }
        ui::end_tooltip();
    }

    ui::same_line();
    ui::set_next_item_width(ui::calc_text_size("should be roughly this long incl label").x);
    float blend = m_State->getWarpBlendingFactor();
    if (ui::draw_float_slider("blending", &blend, 0.0f, 1.0f)) {
        m_State->setWarpBlendingFactor(blend);
    }

    ui::same_line();
    {
        bool v = m_State->isCameraLinked();
        if (ui::draw_checkbox("link cameras", &v)) {
            m_State->setCameraLinked(v);
        }
    }

    ui::same_line();
    {
        bool v = m_State->isOnlyCameraRotationLinked();
        if (ui::draw_checkbox("only link rotation", &v)) {
            m_State->setOnlyCameraRotationLinked(v);
        }
    }
}

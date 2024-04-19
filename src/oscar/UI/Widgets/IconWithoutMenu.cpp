#include "IconWithoutMenu.h"

#include <oscar/UI/Icon.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

#include <string>
#include <utility>

using namespace osc;

osc::IconWithoutMenu::IconWithoutMenu(
    Icon icon,
    CStringView title,
    CStringView description) :

    m_Icon{std::move(icon)},
    m_Title{title},
    m_ButtonID{"##" + m_Title},
    m_Description{description}
{}

Vec2 osc::IconWithoutMenu::dimensions() const
{
    const Vec2 padding = ui::GetStyleFramePadding();
    return Vec2{m_Icon.dimensions()} + 2.0f*padding;
}

bool osc::IconWithoutMenu::onDraw()
{
    const bool rv = ui::ImageButton(m_ButtonID, m_Icon.get_texture(), m_Icon.dimensions(), m_Icon.getTextureCoordinates());
    ui::DrawTooltipIfItemHovered(m_Title, m_Description);
    return rv;
}

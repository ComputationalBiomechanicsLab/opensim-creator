#include "IconWithoutMenu.h"

#include <oscar/UI/Icon.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/Utils/CStringView.h>

#include <string>
#include <utility>

osc::IconWithoutMenu::IconWithoutMenu(
    Icon icon,
    CStringView title,
    CStringView description) :

    m_Icon{std::move(icon)},
    m_Title{title},
    m_ButtonID{"##" + m_Title},
    m_Description{description}
{
}

bool osc::IconWithoutMenu::onDraw()
{
    bool const rv = ImageButton(m_ButtonID, m_Icon.getTexture(), m_Icon.getDimensions(), m_Icon.getTextureCoordinates());
    DrawTooltipIfItemHovered(m_Title, m_Description);
    return rv;
}

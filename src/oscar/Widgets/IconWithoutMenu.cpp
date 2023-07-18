#include "IconWithoutMenu.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Icon.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <string>
#include <utility>

osc::IconWithoutMenu::IconWithoutMenu(
    osc::Icon icon,
    osc::CStringView title,
    osc::CStringView description) :

    m_Icon{std::move(icon)},
    m_Title{title},
    m_ButtonID{"##" + m_Title},
    m_Description{description}
{
}

bool osc::IconWithoutMenu::draw()
{
    bool rv = osc::ImageButton(m_ButtonID, m_Icon.getTexture(), m_Icon.getDimensions(), m_Icon.getTextureCoordinates());
    osc::DrawTooltipIfItemHovered(m_Title, m_Description);
    return rv;
}

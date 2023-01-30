#include "IconWithoutMenu.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Icon.hpp"
#include "src/Utils/CStringView.hpp"

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
    bool rv = osc::ImageButton(m_ButtonID, m_Icon.getTexture(), m_Icon.getDimensions());
    osc::DrawTooltipIfItemHovered(m_Title, m_Description);
    return rv;
}
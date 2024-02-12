#pragma once

#include <oscar/UI/Icon.h>
#include <oscar/Utils/CStringView.h>

#include <string>

namespace osc
{
    class IconWithoutMenu final {
    public:
        IconWithoutMenu(
            Icon icon,
            CStringView title,
            CStringView description
        );

        CStringView getIconID() const
        {
            return m_ButtonID;
        }

        CStringView getTitle() const
        {
            return m_Title;
        }

        bool onDraw();

    private:
        Icon m_Icon;
        std::string m_Title;
        std::string m_ButtonID;
        std::string m_Description;
    };
}

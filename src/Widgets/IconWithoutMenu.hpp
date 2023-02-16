#pragma once

#include "src/Graphics/Icon.hpp"
#include "src/Utils/CStringView.hpp"

#include <string>

namespace osc
{
    class IconWithoutMenu final {
    public:
        IconWithoutMenu(
            osc::Icon icon,
            osc::CStringView title,
            osc::CStringView description
        );

        std::string const& getIconID() const
        {
            return m_ButtonID;
        }

        std::string const& getTitle() const
        {
            return m_Title;
        }

        bool draw();

    private:
        osc::Icon m_Icon;
        std::string m_Title;
        std::string m_ButtonID;
        std::string m_Description;
    };
}
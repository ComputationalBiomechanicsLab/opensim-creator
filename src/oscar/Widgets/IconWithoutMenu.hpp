#pragma once

#include "oscar/Graphics/Icon.hpp"
#include "oscar/Utils/CStringView.hpp"

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
        Icon m_Icon;
        std::string m_Title;
        std::string m_ButtonID;
        std::string m_Description;
    };
}

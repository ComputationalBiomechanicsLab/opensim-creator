#pragma once

#include "oscar/Graphics/Icon.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/Widgets/IconWithoutMenu.hpp"

#include <functional>
#include <string>

namespace osc
{
    class IconWithMenu final {
    public:
        IconWithMenu(
            osc::Icon icon,
            osc::CStringView title,
            osc::CStringView description,
            std::function<void()> contentRenderer
        );

        void draw();
    private:
        IconWithoutMenu m_IconWithoutMenu;
        std::string m_ContextMenuID;
        std::function<void()> m_ContentRenderer;
    };
}

#pragma once

#include <oscar/UI/Icon.h>
#include <oscar/UI/Widgets/IconWithoutMenu.h>
#include <oscar/Utils/CStringView.h>

#include <functional>
#include <string>

namespace osc
{
    class IconWithMenu final {
    public:
        IconWithMenu(
            Icon icon,
            CStringView title,
            CStringView description,
            std::function<bool()> contentRenderer
        );

        bool onDraw();
    private:
        IconWithoutMenu m_IconWithoutMenu;
        std::string m_ContextMenuID;
        std::function<bool()> m_ContentRenderer;
    };
}

#pragma once

#include "oscar/Graphics/Icon.hpp"
#include "oscar/Utils/CStringView.hpp"
#include "oscar/UI/Widgets/IconWithoutMenu.hpp"

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
            std::function<void()> contentRenderer
        );

        void onDraw();
    private:
        IconWithoutMenu m_IconWithoutMenu;
        std::string m_ContextMenuID;
        std::function<void()> m_ContentRenderer;
    };
}

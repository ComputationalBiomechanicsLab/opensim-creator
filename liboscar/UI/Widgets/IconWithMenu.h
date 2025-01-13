#pragma once

#include <liboscar/UI/Icon.h>
#include <liboscar/UI/Widgets/IconWithoutMenu.h>
#include <liboscar/Utils/CStringView.h>

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
            std::function<bool()> content_renderer
        );

        Vec2 dimensions() const { return icon_without_menu_.dimensions(); }
        bool on_draw();
    private:
        IconWithoutMenu icon_without_menu_;
        std::string context_menu_id_;
        std::function<bool()> content_renderer_;
    };
}

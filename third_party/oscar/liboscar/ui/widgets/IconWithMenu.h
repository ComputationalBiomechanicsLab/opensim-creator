#pragma once

#include <liboscar/ui/Icon.h>
#include <liboscar/ui/widgets/IconWithoutMenu.h>
#include <liboscar/utils/CStringView.h>

#include <functional>
#include <string>

namespace osc
{
    class IconWithMenu final {
    public:
        explicit IconWithMenu(
            Icon icon,
            CStringView title,
            CStringView description,
            std::function<bool()> content_renderer
        );

        Vector2 dimensions() const { return icon_without_menu_.dimensions(); }
        bool on_draw();
    private:
        IconWithoutMenu icon_without_menu_;
        std::string context_menu_id_;
        std::function<bool()> content_renderer_;
    };
}

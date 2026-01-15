#pragma once

#include <liboscar/maths/Vector2.h>
#include <liboscar/ui/icon.h>
#include <liboscar/utils/CStringView.h>

#include <string>

namespace osc
{
    class IconWithoutMenu final {
    public:
        explicit IconWithoutMenu(
            Icon icon,
            CStringView title,
            CStringView description
        );

        CStringView icon_id() const { return button_id_; }

        CStringView title() const { return title_; }

        Vector2 dimensions() const;
        bool on_draw();

    private:
        Icon icon_;
        std::string title_;
        std::string button_id_;
        std::string description_;
    };
}

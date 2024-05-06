#pragma once

#include <oscar/Maths/Vec2.h>
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

        CStringView icon_id() const { return button_id_; }

        CStringView title() const { return title_; }

        Vec2 dimensions() const;
        bool on_draw();

    private:
        Icon icon_;
        std::string title_;
        std::string button_id_;
        std::string description_;
    };
}

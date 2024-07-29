#include "IconWithoutMenu.h"

#include <oscar/UI/Icon.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

#include <string>
#include <utility>

using namespace osc;

osc::IconWithoutMenu::IconWithoutMenu(
    Icon icon,
    CStringView title,
    CStringView description) :

    icon_{std::move(icon)},
    title_{title},
    button_id_{"##" + title_},
    description_{description}
{}

Vec2 osc::IconWithoutMenu::dimensions() const
{
    const Vec2 padding = ui::get_style_frame_padding();
    return Vec2{icon_.dimensions()} + 2.0f*padding;
}

bool osc::IconWithoutMenu::on_draw()
{
    const bool rv = ui::draw_image_button(button_id_, icon_.texture(), icon_.dimensions(), icon_.texture_coordinates());
    ui::draw_tooltip_if_item_hovered(title_, description_);
    return rv;
}

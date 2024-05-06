#include "IconWithMenu.h"

#include <oscar/UI/Icon.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/Widgets/IconWithoutMenu.h>
#include <oscar/Utils/CStringView.h>

#include <functional>
#include <string>
#include <utility>

osc::IconWithMenu::IconWithMenu(
    Icon icon,
    CStringView title,
    CStringView description,
    std::function<bool()> content_renderer) :

    icon_without_menu_{std::move(icon), title, description},
    context_menu_id_{"##" + icon_without_menu_.icon_id()},
    content_renderer_{std::move(content_renderer)}
{}

bool osc::IconWithMenu::on_draw()
{
    if (icon_without_menu_.on_draw()) {
        ui::OpenPopup(context_menu_id_);
    }

    bool rv = false;
    if (ui::BeginPopup(context_menu_id_,ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {
        ui::TextDisabled(icon_without_menu_.title());
        ui::Dummy({0.0f, 0.5f*ui::GetTextLineHeight()});
        rv = content_renderer_();
        ui::EndPopup();
    }

    return rv;
}

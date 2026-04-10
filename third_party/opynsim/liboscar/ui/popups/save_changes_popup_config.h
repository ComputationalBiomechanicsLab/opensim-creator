#pragma once

#include <functional>
#include <string>

namespace osc
{
    struct SaveChangesPopupConfig final {
        std::string popup_name = "Save changes?";
        std::function<bool()> on_user_clicked_save = []{ return true; };
        std::function<bool()> on_user_clicked_dont_save = []{ return true; };
        std::function<bool()> on_user_cancelled = []{ return true; };
        std::string content = popup_name;
    };
}

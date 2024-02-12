#pragma once

#include <functional>
#include <string>

namespace osc
{
    struct SaveChangesPopupConfig final {
        std::string popupName = "Save changes?";
        std::function<bool()> onUserClickedSave = []{ return true; };
        std::function<bool()> onUserClickedDontSave = []{ return true; };
        std::function<bool()> onUserCancelled = []{ return true; };
        std::string content = popupName;
    };
}

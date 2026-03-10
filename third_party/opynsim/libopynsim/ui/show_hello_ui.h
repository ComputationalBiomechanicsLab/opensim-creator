#pragma once

#include <libopynsim/ui/ui_callbacks.h>

namespace opyn { class OPynSimApp; }

namespace opyn
{
    void show_hello_ui(OPynSimApp&, UiCallbacks = {});
}

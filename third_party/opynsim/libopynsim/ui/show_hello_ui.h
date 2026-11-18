#pragma once

#include <libopynsim/ui/ui_callbacks.h>

namespace opyn
{
    class OPynSimApp;

    void show_hello_ui(OPynSimApp&, UiCallbacks = {});
}

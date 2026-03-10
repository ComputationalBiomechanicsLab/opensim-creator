#pragma once

#include <libopynsim/ui/ui_callbacks.h>

namespace opyn { class OPynSimApp; }
namespace opyn { class Model; }
namespace opyn { class ModelState; }

namespace opyn
{
    void show_model_in_state(
        OPynSimApp&,
        const Model&,
        const ModelState&,
        bool zoom_to_fit,
        UiCallbacks = {}
    );
}

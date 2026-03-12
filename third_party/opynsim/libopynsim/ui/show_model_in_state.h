#pragma once

#include <libopynsim/ui/ui_callbacks.h>

#include <liboscar/graphics/color.h>
#include <liboscar/maths/vector.h>

namespace opyn { class OPynSimApp; }
namespace opyn { class Model; }
namespace opyn { class ModelState; }

namespace opyn
{
    void show_model_in_state(
        OPynSimApp&,
        const Model&,
        const ModelState&,
        osc::Vector2 dimensions,
        osc::Color background_color,
        bool zoom_to_fit,
        bool show_floor,
        UiCallbacks = {}
    );
}

#pragma once

#include <liboscar/graphics/texture2d.h>
#include <liboscar/maths/vector.h>

namespace opyn { class OPynSimApp; }
namespace opyn { class Model; }
namespace opyn { class ModelState; }

namespace opyn
{
    osc::Texture2D render_model_in_state(
        OPynSimApp&,
        const Model&,
        const ModelState&,
        osc::Vector2 dimensions,
        bool zoom_to_fit,
        bool draw_floor
    );
}

#pragma once

#include <liboscar/graphics/texture2d.h>

#include <utility>

namespace opyn { class OPynSimApp; }
namespace opyn { class Model; }
namespace opyn { class ModelState; }

namespace opyn
{
    osc::Texture2D render_model_in_state(
        OPynSimApp&,
        const Model&,
        const ModelState&,
        std::pair<int, int> dimensions,
        bool zoom_to_fit
    );
}

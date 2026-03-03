#pragma once

#include <liboscar/graphics/texture2d.h>

#include <utility>

namespace opyn { class Model; }
namespace opyn { class ModelState; }

namespace opyn
{
    osc::Texture2D render_model_in_state(
        const Model& model,
        const ModelState& model_state,
        std::pair<int, int> dimensions,
        bool zoom_to_fit
    );
}

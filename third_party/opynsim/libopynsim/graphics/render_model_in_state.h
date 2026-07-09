#pragma once

#include <liboscar/graphics/texture2d.h>
#include <liboscar/maths/vector.h>

namespace opyn { class OPynSimApp; }
namespace opyn { class Model; }
namespace opyn { class ModelState; }
namespace osc { class SceneCache; }
namespace osc { class CameraV2; }

namespace opyn
{
    osc::Texture2D render_model_in_state(
        OPynSimApp&,
        const Model&,
        const ModelState&,
        osc::Vector2 dimensions,
        osc::Color background_color,
        bool draw_floor,
        osc::SceneCache* scene_cache,
        const osc::CameraV2* camera
    );
}

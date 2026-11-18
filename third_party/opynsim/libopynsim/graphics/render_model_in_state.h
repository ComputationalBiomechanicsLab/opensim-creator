#pragma once

#include <liboscar/graphics/texture2d.h>
#include <liboscar/maths/vector.h>

namespace osc
{
    class Camera;
    class SceneCache;
}

namespace opyn
{
    class Model;
    class ModelState;
    class OPynSimApp;

    osc::Texture2D render_model_in_state(
        OPynSimApp&,
        const Model&,
        const ModelState&,
        osc::Vector2 dimensions,
        osc::Color background_color,
        bool draw_floor,
        osc::SceneCache* scene_cache,
        const osc::Camera* camera);
}

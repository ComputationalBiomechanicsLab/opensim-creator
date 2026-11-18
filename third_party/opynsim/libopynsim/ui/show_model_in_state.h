#pragma once

#include <libopynsim/ui/ui_callbacks.h>

#include <liboscar/graphics/color.h>
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

    void show_model_in_state(
        OPynSimApp&,
        const Model&,
        const ModelState&,
        osc::Vector2 dimensions,
        osc::Color background_color,
        bool draw_floor,
        osc::SceneCache* scene_cache,
        osc::Camera* camera,
        UiCallbacks = {});
}

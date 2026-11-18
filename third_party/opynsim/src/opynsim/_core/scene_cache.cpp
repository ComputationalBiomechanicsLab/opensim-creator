#include "scene_cache.h"

#include <liboscar/graphics/scene/scene_cache.h>
#include <nanobind/nanobind.h>

namespace nb = nanobind;

void opyn::python::def_scene_cache(nanobind::module_& m)
{
    nb::class_<osc::SceneCache> cls(
        m,
        "SceneCache",
        R"(
            Caches slow-to-load assets (shaders, meshes).
        )");
    cls.def(nb::init<>{});
}

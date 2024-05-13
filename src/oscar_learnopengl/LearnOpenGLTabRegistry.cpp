#include "LearnOpenGLTabRegistry.h"

#include <oscar_learnopengl/LearnOpenGLTabs.h>

#include <oscar/UI/Tabs/TabRegistry.h>

void osc::register_learnopengl_tabs(TabRegistry& registry)
{
    // register each concrete tab with the registry
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.register_tab<Tabs>(), ...);
    }(LearnOpenGLTabs{});
}

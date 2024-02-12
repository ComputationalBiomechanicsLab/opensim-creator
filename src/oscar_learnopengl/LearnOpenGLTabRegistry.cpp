#include "LearnOpenGLTabRegistry.h"

#include <oscar_learnopengl/LearnOpenGLTabs.h>

#include <oscar/UI/Tabs/TabRegistry.h>

void osc::RegisterLearnOpenGLTabs(TabRegistry& registry)
{
    // register each concrete tab with the registry
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.registerTab<Tabs>(), ...);
    }(LearnOpenGLTabs{});
}

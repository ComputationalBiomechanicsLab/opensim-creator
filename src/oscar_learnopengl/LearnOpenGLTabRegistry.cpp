#include "LearnOpenGLTabRegistry.hpp"

#include <oscar_learnopengl/LearnOpenGLTabs.hpp>

#include <oscar/UI/Tabs/TabRegistry.hpp>

void osc::RegisterLearnOpenGLTabs(TabRegistry& registry)
{
    // register each concrete tab with the registry
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.registerTab<Tabs>(), ...);
    }(LearnOpenGLTabs{});
}

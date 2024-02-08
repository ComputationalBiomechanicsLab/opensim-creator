#include "OpenSimCreatorTabRegistry.hpp"

#include <OpenSimCreator/UI/OpenSimCreatorTabs.hpp>

#include <oscar/UI/Tabs/TabRegistry.hpp>

void osc::RegisterOpenSimCreatorTabs(TabRegistry& registry)
{
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.registerTab<Tabs>(), ...);
    }(OpenSimCreatorTabs{});
}

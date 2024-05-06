#include "OpenSimCreatorTabRegistry.h"

#include <OpenSimCreator/UI/OpenSimCreatorTabs.h>

#include <oscar/UI/Tabs/TabRegistry.h>

void osc::RegisterOpenSimCreatorTabs(TabRegistry& registry)
{
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.register_tab<Tabs>(), ...);
    }(OpenSimCreatorTabs{});
}

#include "OpenSimCreatorTabRegistry.h"

#include <libOpenSimCreator/UI/OpenSimCreatorTabs.h>

#include <liboscar/UI/Tabs/TabRegistry.h>

void osc::RegisterOpenSimCreatorTabs(TabRegistry& registry)
{
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.register_tab<Tabs>(), ...);
    }(OpenSimCreatorTabs{});
}

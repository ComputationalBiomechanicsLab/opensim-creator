#include "OscarDemosTabRegistry.h"

#include <oscar_demos/OscarDemoTabs.h>

#include <oscar/UI/Tabs/TabRegistry.h>

void osc::RegisterDemoTabs(TabRegistry& registry)
{
    // register each concrete tab with the registry
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.registerTab<Tabs>(), ...);
    }(OscarDemoTabs{});
}

#include "OscarDemosTabRegistry.h"

#include <oscar_demos/OscarDemoTabs.h>

#include <oscar/UI/Tabs/TabRegistry.h>

void osc::register_demo_tabs(TabRegistry& registry)
{
    // register each concrete tab with the registry
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.register_tab<Tabs>(), ...);
    }(OscarDemoTabs{});
}

#include "OscarDemosTabRegistry.h"

#include <liboscar-demos/OscarDemoTabs.h>

#include <liboscar/ui/tabs/tab_registry.h>

void osc::register_demo_tabs(TabRegistry& registry)
{
    // register each concrete tab with the registry
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.register_tab<Tabs>(), ...);
    }(OscarDemoTabs{});
}

#include "OscarDemosTabRegistry.hpp"

#include <oscar_demos/OscarDemoTabs.hpp>

#include <oscar/UI/Tabs/TabRegistry.hpp>

void osc::RegisterDemoTabs(TabRegistry& registry)
{
    // register each concrete tab with the registry
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.registerTab<Tabs>(), ...);
    }(OscarDemoTabs{});
}

#include "OscarSimbodyTabRegistry.h"

#include <oscar_simbody/UI/OscarSimbodyTabs.h>

#include <oscar/UI/Tabs/TabRegistry.h>

void osc::RegisterOscarSimbodyTabs(TabRegistry& registry)
{
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.register_tab<Tabs>(), ...);
    }(OscarSimbodyTabs{});
}

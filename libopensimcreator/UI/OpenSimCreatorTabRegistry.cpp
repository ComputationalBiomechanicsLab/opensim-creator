#include "OpenSimCreatorTabRegistry.h"

#include <libopensimcreator/UI/OpenSimCreatorTabs.h>

#include <liboscar/ui/tabs/TabRegistry.h>

void osc::RegisterOpenSimCreatorTabs(TabRegistry& registry)
{
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.register_tab<Tabs>(), ...);
    }(OpenSimCreatorTabs{});
}

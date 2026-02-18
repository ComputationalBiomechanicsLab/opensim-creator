#include "open_sim_creator_tab_registry.h"

#include <libopensimcreator/ui/open_sim_creator_tabs.h>

#include <liboscar/ui/tabs/tab_registry.h>

void osc::RegisterOpenSimCreatorTabs(TabRegistry& registry)
{
    [&registry]<typename... Tabs>(Typelist<Tabs...>)
    {
        (registry.register_tab<Tabs>(), ...);
    }(OpenSimCreatorTabs{});
}

#include "BookOfShadersTabRegistry.h"

#include <oscar_bookofshaders/BookOfShadersTab.h>

#include <oscar/UI/Tabs/TabRegistry.h>

void osc::register_bookofshaders_tabs(TabRegistry& registry)
{
    registry.register_tab<BookOfShadersTab>();
}

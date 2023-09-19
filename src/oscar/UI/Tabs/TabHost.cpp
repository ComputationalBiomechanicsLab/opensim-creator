#include "TabHost.hpp"

#include <oscar/UI/Tabs/Tab.hpp>
#include <oscar/Utils/UID.hpp>

#include <memory>

// the impl-forwarding methods are here so that it's easier to hook into
// the API (e.g. for debugging)


// public API

osc::UID osc::TabHost::addTab(std::unique_ptr<Tab> tab)
{
    return implAddTab(std::move(tab));
}

void osc::TabHost::selectTab(UID tabID)
{
    implSelectTab(tabID);
}

void osc::TabHost::closeTab(UID tabID)
{
    implCloseTab(tabID);
}

void osc::TabHost::resetImgui()
{
    implResetImgui();
}

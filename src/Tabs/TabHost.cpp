#include "TabHost.hpp"

#include "src/Tabs/Tab.hpp"
#include "src/Utils/UID.hpp"

#include <memory>

// the impl-forwarding methods are here so that it's easier to hook into
// the API (e.g. for debugging)


// public API

void osc::TabHost::addTab(std::unique_ptr<Tab> tab)
{
	implAddTab(std::move(tab));
}

void osc::TabHost::selectTab(UID tabID)
{
	implSelectTab(std::move(tabID));
}

void osc::TabHost::closeTab(UID tabID)
{
	implCloseTab(std::move(tabID));
}
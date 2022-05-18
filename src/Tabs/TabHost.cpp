#include "TabHost.hpp"

#include "src/Tabs/Tab.hpp"

#include <memory>

// the impl-forwarding methods are here so that it's easier to hook into
// the API (e.g. for debugging)

// public API

void osc::TabHost::addTab(std::unique_ptr<Tab> tab)
{
	implAddTab(std::move(tab));
}

void osc::TabHost::selectTab(Tab* t)
{
	implSelectTab(std::move(t));
}

void osc::TabHost::closeTab(Tab* t)
{
	implCloseTab(std::move(t));
}
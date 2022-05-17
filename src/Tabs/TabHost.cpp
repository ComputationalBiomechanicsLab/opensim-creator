#include "TabHost.hpp"

#include "src/Tabs/Tab.hpp"

#include <memory>

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
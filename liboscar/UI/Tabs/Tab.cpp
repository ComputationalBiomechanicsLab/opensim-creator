#include "Tab.h"

#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/Utils/UID.h>

#include <memory>

using namespace osc;

osc::Tab::Tab(std::unique_ptr<TabPrivate>&& ptr) :
    Widget{std::move(ptr)}
{}
UID osc::Tab::id() const { return private_data().id(); }

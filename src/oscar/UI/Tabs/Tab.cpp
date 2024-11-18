#include "Tab.h"

#include <oscar/UI/Tabs/TabPrivate.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/UID.h>

#include <memory>

using namespace osc;

osc::Tab::Tab(std::unique_ptr<TabPrivate>&& ptr) :
    Widget{std::move(ptr)}
{}
UID osc::Tab::id() const { return private_data().id(); }

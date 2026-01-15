#include "tab.h"

#include <liboscar/ui/tabs/tab_private.h>
#include <liboscar/utils/UID.h>

#include <future>
#include <memory>

using namespace osc;

osc::Tab::Tab(std::unique_ptr<TabPrivate>&& ptr) :
    Widget{std::move(ptr)}
{}
std::future<TabSaveResult> osc::Tab::impl_try_save()
{
    std::promise<TabSaveResult> p;
    p.set_value(TabSaveResult::Done);
    return p.get_future();
}
UID osc::Tab::id() const { return private_data().id(); }

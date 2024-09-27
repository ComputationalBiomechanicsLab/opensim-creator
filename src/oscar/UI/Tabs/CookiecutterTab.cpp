#include "CookiecutterTab.h"

#include <oscar/UI/Tabs/TabPrivate.h>
#include <oscar/Utils/CStringView.h>

#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "CookiecutterTab";
}

class osc::CookiecutterTab::Impl final : public TabPrivate {
public:
    Impl() : TabPrivate{c_tab_string_id} {}

    void on_mount() {}
    void on_unmount() {}
    bool on_event(Event&) { return false; }
    void on_tick() {}
    void on_draw_main_menu() {}
    void on_draw() {}
};


osc::CStringView osc::CookiecutterTab::id() { return c_tab_string_id; }

osc::CookiecutterTab::CookiecutterTab(const ParentPtr<ITabHost>&) :
    Tab{std::make_unique<Impl>()}
{}
void osc::CookiecutterTab::impl_on_mount() { private_data().on_mount(); }
void osc::CookiecutterTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::CookiecutterTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::CookiecutterTab::impl_on_tick() { private_data().on_tick(); }
void osc::CookiecutterTab::impl_on_draw_main_menu() { private_data().on_draw_main_menu(); }
void osc::CookiecutterTab::impl_on_draw() { private_data().on_draw(); }

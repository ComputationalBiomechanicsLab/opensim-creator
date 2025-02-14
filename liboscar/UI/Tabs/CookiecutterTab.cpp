#include "CookiecutterTab.h"

#include <liboscar/UI/Tabs/TabPrivate.h>
#include <liboscar/Utils/CStringView.h>

#include <memory>

using namespace osc;

class osc::CookiecutterTab::Impl final : public TabPrivate {
public:
    static CStringView static_label() { return "CookiecutterTab"; }

    explicit Impl(CookiecutterTab& owner, Widget* parent) :
        TabPrivate{owner, parent, static_label()}
    {}

    void on_mount() {}
    void on_unmount() {}
    bool on_event(Event&) { return false; }
    void on_tick() {}
    void on_draw_main_menu() {}
    void on_draw() {}
};

osc::CStringView osc::CookiecutterTab::id() { return Impl::static_label(); }

osc::CookiecutterTab::CookiecutterTab(Widget* parent) :
    Tab{std::make_unique<Impl>(*this, parent)}
{}
void osc::CookiecutterTab::impl_on_mount() { private_data().on_mount(); }
void osc::CookiecutterTab::impl_on_unmount() { private_data().on_unmount(); }
bool osc::CookiecutterTab::impl_on_event(Event& e) { return private_data().on_event(e); }
void osc::CookiecutterTab::impl_on_tick() { private_data().on_tick(); }
void osc::CookiecutterTab::impl_on_draw_main_menu() { private_data().on_draw_main_menu(); }
void osc::CookiecutterTab::impl_on_draw() { private_data().on_draw(); }

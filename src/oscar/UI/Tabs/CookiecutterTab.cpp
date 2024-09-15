#include "CookiecutterTab.h"

#include <oscar/UI/Tabs/StandardTabImpl.h>
#include <oscar/Utils/CStringView.h>

#include <memory>

using namespace osc;

namespace
{
    constexpr CStringView c_tab_string_id = "CookiecutterTab";
}

class osc::CookiecutterTab::Impl final : public StandardTabImpl {
public:
    Impl() : StandardTabImpl{c_tab_string_id}
    {}

private:
    void impl_on_mount() final
    {}

    void impl_on_unmount() final
    {}

    bool impl_on_event(const Event&) final
    {
        return false;
    }

    void impl_on_tick() final
    {}

    void impl_on_draw_main_menu() final
    {}

    void impl_on_draw() final
    {}
};


osc::CStringView osc::CookiecutterTab::id()
{
    return c_tab_string_id;
}

osc::CookiecutterTab::CookiecutterTab(const ParentPtr<ITabHost>&) :
    impl_{std::make_unique<Impl>()}
{}
osc::CookiecutterTab::CookiecutterTab(CookiecutterTab&&) noexcept = default;
osc::CookiecutterTab& osc::CookiecutterTab::operator=(CookiecutterTab&&) noexcept = default;
osc::CookiecutterTab::~CookiecutterTab() noexcept = default;

UID osc::CookiecutterTab::impl_get_id() const
{
    return impl_->id();
}

CStringView osc::CookiecutterTab::impl_get_name() const
{
    return impl_->name();
}

void osc::CookiecutterTab::impl_on_mount()
{
    impl_->on_mount();
}

void osc::CookiecutterTab::impl_on_unmount()
{
    impl_->on_unmount();
}

bool osc::CookiecutterTab::impl_on_event(Event const& e)
{
    return impl_->on_event(e);
}

void osc::CookiecutterTab::impl_on_tick()
{
    impl_->on_tick();
}

void osc::CookiecutterTab::impl_on_draw_main_menu()
{
    impl_->on_draw_main_menu();
}

void osc::CookiecutterTab::impl_on_draw()
{
    impl_->on_draw();
}

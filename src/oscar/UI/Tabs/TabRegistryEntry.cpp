#include "TabRegistryEntry.h"

#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/CStringView.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

using namespace osc;

class osc::TabRegistryEntry::Impl final {
public:

    Impl(
        CStringView name,
        std::function<std::unique_ptr<Tab>(Widget&)> tab_constructor) :

        name_{name},
        tab_constructor_{std::move(tab_constructor)}
    {}

    CStringView name() const
    {
        return name_;
    }

    std::unique_ptr<Tab> construct_tab(Widget& host) const
    {
        return tab_constructor_(host);
    }

private:
    std::string name_;
    std::function<std::unique_ptr<Tab>(Widget&)> tab_constructor_;
};

osc::TabRegistryEntry::TabRegistryEntry(
    CStringView name,
    std::function<std::unique_ptr<Tab>(Widget&)> tab_constructor) :

    impl_{std::make_shared<Impl>(name, std::move(tab_constructor))}
{}

CStringView osc::TabRegistryEntry::name() const
{
    return impl_->name();
}

std::unique_ptr<Tab> osc::TabRegistryEntry::construct_tab(Widget& host) const
{
    return impl_->construct_tab(host);
}

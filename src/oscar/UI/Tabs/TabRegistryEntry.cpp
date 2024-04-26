#include "TabRegistryEntry.h"

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/Utils/CStringView.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

using namespace osc;

class osc::TabRegistryEntry::Impl final {
public:

    Impl(
        CStringView name_,
        std::function<std::unique_ptr<ITab>(const ParentPtr<ITabHost>&)> constructor_) :

        m_Name{name_},
        m_Constructor{std::move(constructor_)}
    {}

    CStringView getName() const
    {
        return m_Name;
    }

    std::unique_ptr<ITab> createTab(const ParentPtr<ITabHost>& host) const
    {
        return m_Constructor(host);
    }

private:
    std::string m_Name;
    std::function<std::unique_ptr<ITab>(const ParentPtr<ITabHost>&)> m_Constructor;
};

osc::TabRegistryEntry::TabRegistryEntry(
    CStringView name_,
    std::function<std::unique_ptr<ITab>(const ParentPtr<ITabHost>&)> ctor_) :

    m_Impl{std::make_shared<Impl>(name_, std::move(ctor_))}
{}

osc::TabRegistryEntry::TabRegistryEntry(const TabRegistryEntry&) = default;
osc::TabRegistryEntry::TabRegistryEntry(TabRegistryEntry&&) noexcept = default;
osc::TabRegistryEntry& osc::TabRegistryEntry::operator=(const TabRegistryEntry&) = default;
osc::TabRegistryEntry& osc::TabRegistryEntry::operator=(TabRegistryEntry&&) noexcept = default;
osc::TabRegistryEntry::~TabRegistryEntry() noexcept = default;

CStringView osc::TabRegistryEntry::getName() const
{
    return m_Impl->getName();
}

std::unique_ptr<ITab> osc::TabRegistryEntry::createTab(const ParentPtr<ITabHost>& host) const
{
    return m_Impl->createTab(host);
}

#include "TabRegistryEntry.hpp"

#include <oscar/UI/Tabs/Tab.hpp>
#include <oscar/Utils/CStringView.hpp>

#include <functional>
#include <memory>
#include <string>
#include <utility>

class osc::TabRegistryEntry::Impl final {
public:

    Impl(
        CStringView name_,
        std::function<std::unique_ptr<Tab>(ParentPtr<TabHost> const&)> constructor_) :

        m_Name{name_},
        m_Constructor{std::move(constructor_)}
    {
    }

    CStringView getName() const
    {
        return m_Name;
    }

    std::unique_ptr<Tab> createTab(ParentPtr<TabHost> const& host) const
    {
        return m_Constructor(host);
    }

private:
    std::string m_Name;
    std::function<std::unique_ptr<Tab>(ParentPtr<TabHost> const&)> m_Constructor;
};

osc::TabRegistryEntry::TabRegistryEntry(
    CStringView name_,
    std::function<std::unique_ptr<Tab>(ParentPtr<TabHost> const&)> ctor_) :

    m_Impl{std::make_shared<Impl>(name_, std::move(ctor_))}
{
}

osc::TabRegistryEntry::TabRegistryEntry(TabRegistryEntry const&) = default;
osc::TabRegistryEntry::TabRegistryEntry(TabRegistryEntry&&) noexcept = default;
osc::TabRegistryEntry& osc::TabRegistryEntry::operator=(TabRegistryEntry const&) = default;
osc::TabRegistryEntry& osc::TabRegistryEntry::operator=(TabRegistryEntry&&) noexcept = default;
osc::TabRegistryEntry::~TabRegistryEntry() noexcept = default;

osc::CStringView osc::TabRegistryEntry::getName() const
{
    return m_Impl->getName();
}

std::unique_ptr<osc::Tab> osc::TabRegistryEntry::createTab(ParentPtr<TabHost> const& host) const
{
    return m_Impl->createTab(host);
}

bool osc::operator<(TabRegistryEntry const& lhs, TabRegistryEntry const& rhs)
{
    return lhs.getName() < rhs.getName();
}

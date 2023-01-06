#include "TabRegistryEntry.hpp"

#include "src/Tabs/Tab.hpp"
#include "src/Utils/CStringView.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

class osc::TabRegistryEntry::Impl final {
public:

    Impl(CStringView name_, std::function<std::unique_ptr<Tab>(TabHost*)> constructor_) :
        m_Name{std::move(name_)},
        m_Constructor{std::move(constructor_)}
    {
    }

    CStringView getName() const
    {
        return m_Name;
    }

    std::unique_ptr<Tab> createTab(TabHost* host) const
    {
        return m_Constructor(std::move(host));
    }

private:
    std::string m_Name;
    std::function<std::unique_ptr<Tab>(TabHost*)> m_Constructor;
};

osc::TabRegistryEntry::TabRegistryEntry(CStringView name_, std::function<std::unique_ptr<Tab>(TabHost*)> ctor_) :
    m_Impl{std::make_shared<Impl>(std::move(name_), std::move(ctor_))}
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

std::unique_ptr<osc::Tab> osc::TabRegistryEntry::createTab(TabHost* host) const
{
    return m_Impl->createTab(std::move(host));
}

bool osc::operator<(TabRegistryEntry const& a, TabRegistryEntry const& b) noexcept
{
    return a.getName() < b.getName();
}
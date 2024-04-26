#include "TabRegistry.h"

#include <oscar/Utils/Algorithms.h>

#include <algorithm>
#include <memory>
#include <ranges>
#include <string>
#include <vector>

using namespace osc;
namespace rgs = std::ranges;

class osc::TabRegistry::Impl final {
public:
    void registerTab(const TabRegistryEntry& newEntry)
    {
        m_Entries.push_back(newEntry);
        std::sort(m_Entries.begin(), m_Entries.end());
    }

    size_t size() const
    {
        return m_Entries.size();
    }

    TabRegistryEntry operator[](size_t i) const
    {
        return m_Entries[i];
    }

    std::optional<TabRegistryEntry> getByName(std::string_view name) const
    {
        const auto it = rgs::find(m_Entries, name, &TabRegistryEntry::getName);
        return it != m_Entries.end() ? *it : std::optional<TabRegistryEntry>{};
    }

private:
    std::vector<TabRegistryEntry> m_Entries;
};


// public API (PIMPL)

osc::TabRegistry::TabRegistry() :
    m_Impl{std::make_unique<Impl>()}
{
}
osc::TabRegistry::TabRegistry(TabRegistry&&) noexcept = default;
osc::TabRegistry& osc::TabRegistry::operator=(TabRegistry&&) noexcept = default;
osc::TabRegistry::~TabRegistry() noexcept = default;

void osc::TabRegistry::registerTab(const TabRegistryEntry& newEntry)
{
    m_Impl->registerTab(newEntry);
}

size_t osc::TabRegistry::size() const
{
    return m_Impl->size();
}

TabRegistryEntry osc::TabRegistry::operator[](size_t i) const
{
    return (*m_Impl)[i];
}

std::optional<TabRegistryEntry> osc::TabRegistry::getByName(std::string_view name) const
{
    return m_Impl->getByName(name);
}

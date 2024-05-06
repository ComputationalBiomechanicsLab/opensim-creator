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
    void register_tab(const TabRegistryEntry& entry)
    {
        entries_.push_back(entry);
        rgs::sort(entries_, rgs::less{}, &TabRegistryEntry::name);
    }

    size_t size() const
    {
        return entries_.size();
    }

    TabRegistryEntry operator[](size_t pos) const
    {
        return entries_[pos];
    }

    std::optional<TabRegistryEntry> find_by_name(std::string_view name) const
    {
        const auto it = rgs::find(entries_, name, &TabRegistryEntry::name);
        return it != entries_.end() ? *it : std::optional<TabRegistryEntry>{};
    }

private:
    std::vector<TabRegistryEntry> entries_;
};

osc::TabRegistry::TabRegistry() :
    impl_{std::make_unique<Impl>()}
{}
osc::TabRegistry::TabRegistry(TabRegistry&&) noexcept = default;
osc::TabRegistry& osc::TabRegistry::operator=(TabRegistry&&) noexcept = default;
osc::TabRegistry::~TabRegistry() noexcept = default;

void osc::TabRegistry::register_tab(const TabRegistryEntry& entry)
{
    impl_->register_tab(entry);
}

size_t osc::TabRegistry::size() const
{
    return impl_->size();
}

TabRegistryEntry osc::TabRegistry::operator[](size_t pos) const
{
    return (*impl_)[pos];
}

std::optional<TabRegistryEntry> osc::TabRegistry::find_by_name(std::string_view name) const
{
    return impl_->find_by_name(name);
}

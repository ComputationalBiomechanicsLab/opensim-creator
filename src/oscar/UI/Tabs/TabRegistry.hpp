#pragma once

#include "oscar/UI/Tabs/TabRegistryEntry.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>

namespace osc
{
    // container for alphabetically-sorted tab entries
    class TabRegistry final {
    public:
        TabRegistry();
        TabRegistry(TabRegistry const&) = delete;
        TabRegistry(TabRegistry&&) noexcept;
        TabRegistry& operator=(TabRegistry const&) = delete;
        TabRegistry& operator=(TabRegistry&&) noexcept;
        ~TabRegistry() noexcept;

        void registerTab(TabRegistryEntry const&);
        size_t size() const;
        TabRegistryEntry operator[](size_t) const;
        std::optional<TabRegistryEntry> getByName(std::string_view) const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

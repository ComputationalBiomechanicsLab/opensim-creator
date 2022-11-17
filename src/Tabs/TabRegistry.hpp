#pragma once

#include "src/Utils/CStringView.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace osc { class Tab; }
namespace osc { class TabHost; }

namespace osc
{
    class TabRegistryEntry final {
    public:
        TabRegistryEntry(CStringView name_, std::unique_ptr<Tab>(*ctor_)(TabHost*));
        TabRegistryEntry(TabRegistryEntry const&) = default;
        TabRegistryEntry(TabRegistryEntry&&) noexcept = default;
        TabRegistryEntry& operator=(TabRegistryEntry const&) = default;
        TabRegistryEntry& operator=(TabRegistryEntry&&) = default;
        ~TabRegistryEntry() noexcept;

        CStringView getName() const;
        std::unique_ptr<Tab> createTab(TabHost* host) const;

    private:
        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator<(TabRegistryEntry const&, TabRegistryEntry const&);

    ptrdiff_t GetNumRegisteredTabs();
    TabRegistryEntry GetRegisteredTab(ptrdiff_t);
    std::optional<TabRegistryEntry> GetRegisteredTabByName(std::string_view);
    bool RegisterTab(CStringView name, std::unique_ptr<Tab>(*)(TabHost*));
}
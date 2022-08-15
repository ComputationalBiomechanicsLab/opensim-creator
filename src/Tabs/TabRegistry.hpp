#pragma once

#include "src/Utils/CStringView.hpp"

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
        TabRegistryEntry(TabRegistryEntry const&);
        TabRegistryEntry(TabRegistryEntry&&) noexcept;
        TabRegistryEntry& operator=(TabRegistryEntry const&);
        TabRegistryEntry& operator=(TabRegistryEntry&&);
        ~TabRegistryEntry() noexcept;

        CStringView getName() const;
        std::unique_ptr<Tab> createTab(TabHost* host) const;

    private:
        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator<(TabRegistryEntry const&, TabRegistryEntry const&);

    int GetNumRegisteredTabs();
    TabRegistryEntry GetRegisteredTab(int);
    std::optional<TabRegistryEntry> GetRegisteredTabByName(std::string_view);
    bool RegisterTab(CStringView name, std::unique_ptr<Tab>(*ctor_)(TabHost*));
}
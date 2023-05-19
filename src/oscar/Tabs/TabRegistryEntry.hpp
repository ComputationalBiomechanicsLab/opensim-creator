#pragma once

#include "oscar/Utils/CStringView.hpp"

#include <functional>
#include <memory>

namespace osc { class Tab; }
namespace osc { class TabHost; }

namespace osc
{
    // reference-counted definition for an available tab
    class TabRegistryEntry final {
    public:
        TabRegistryEntry(
            CStringView name_,
            std::function<std::unique_ptr<Tab>(std::weak_ptr<TabHost>)> constructor_
        );
        TabRegistryEntry(TabRegistryEntry const&);
        TabRegistryEntry(TabRegistryEntry&&) noexcept;
        TabRegistryEntry& operator=(TabRegistryEntry const&);
        TabRegistryEntry& operator=(TabRegistryEntry&&) noexcept;
        ~TabRegistryEntry() noexcept;

        CStringView getName() const;
        std::unique_ptr<Tab> createTab(std::weak_ptr<TabHost>) const;

    private:
        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };
    bool operator<(TabRegistryEntry const&, TabRegistryEntry const&) noexcept;
}
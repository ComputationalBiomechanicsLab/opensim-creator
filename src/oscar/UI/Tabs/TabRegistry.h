#pragma once

#include <oscar/UI/Tabs/ITab.h>
#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/UI/Tabs/TabRegistryEntry.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/ParentPtr.h>

#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>

namespace osc
{
    template<typename T>
    concept StandardRegisterableTab =
        std::derived_from<T, ITab> and
        std::constructible_from<T, const ParentPtr<ITabHost>&> and
        requires (T) {
            { T::id() } -> std::same_as<CStringView>;
        };

    // container for alphabetically-sorted tab entries
    class TabRegistry final {
    public:
        TabRegistry();
        TabRegistry(const TabRegistry&) = delete;
        TabRegistry(TabRegistry&&) noexcept;
        TabRegistry& operator=(const TabRegistry&) = delete;
        TabRegistry& operator=(TabRegistry&&) noexcept;
        ~TabRegistry() noexcept;

        void register_tab(const TabRegistryEntry&);

        template<StandardRegisterableTab T>
        void register_tab()
        {
            register_tab(TabRegistryEntry{
                T::id(),
                [](const ParentPtr<ITabHost>& host_ptr) { return std::make_unique<T>(host_ptr); },
            });
        }

        size_t size() const;
        TabRegistryEntry operator[](size_t) const;
        std::optional<TabRegistryEntry> find_by_name(std::string_view) const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

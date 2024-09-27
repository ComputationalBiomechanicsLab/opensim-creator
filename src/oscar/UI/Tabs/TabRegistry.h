#pragma once

#include <oscar/UI/Tabs/ITabHost.h>
#include <oscar/UI/Tabs/Tab.h>
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
        std::derived_from<T, Tab> and
        std::constructible_from<T, const ParentPtr<ITabHost>&> and
        requires (T) {
            { T::id() } -> std::same_as<CStringView>;
        };

    // container for alphabetically-sorted tab entries
    class TabRegistry final {
    public:
        using value_type = TabRegistryEntry;
        using size_type = size_t;
        using iterator = const TabRegistryEntry*;
        using const_iterator = const TabRegistryEntry*;

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

        const_iterator begin() const;
        const_iterator end() const;
        [[nodiscard]] bool empty() const { return size() <= 0; }
        size_type size() const;
        TabRegistryEntry operator[](size_type) const;
        std::optional<TabRegistryEntry> find_by_name(std::string_view) const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };
}

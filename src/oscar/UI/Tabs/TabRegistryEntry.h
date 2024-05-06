#pragma once

#include <oscar/Utils/CStringView.h>

#include <functional>
#include <memory>

namespace osc { template<typename T> class ParentPtr; }
namespace osc { class ITab; }
namespace osc { class ITabHost; }

namespace osc
{
    // reference-counted definition for an available tab
    class TabRegistryEntry final {
    public:
        TabRegistryEntry(
            CStringView name,
            std::function<std::unique_ptr<ITab>(const ParentPtr<ITabHost>&)> tab_constructor
        );
        TabRegistryEntry(const TabRegistryEntry&);
        TabRegistryEntry(TabRegistryEntry&&) noexcept;
        TabRegistryEntry& operator=(const TabRegistryEntry&);
        TabRegistryEntry& operator=(TabRegistryEntry&&) noexcept;
        ~TabRegistryEntry() noexcept;

        CStringView name() const;
        std::unique_ptr<ITab> construct_tab(const ParentPtr<ITabHost>&) const;

    private:
        class Impl;
        std::shared_ptr<Impl> impl_;
    };
}

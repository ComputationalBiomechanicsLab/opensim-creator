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
            CStringView name_,
            std::function<std::unique_ptr<ITab>(const ParentPtr<ITabHost>&)> constructor_
        );
        TabRegistryEntry(const TabRegistryEntry&);
        TabRegistryEntry(TabRegistryEntry&&) noexcept;
        TabRegistryEntry& operator=(const TabRegistryEntry&);
        TabRegistryEntry& operator=(TabRegistryEntry&&) noexcept;
        ~TabRegistryEntry() noexcept;

        CStringView getName() const;
        std::unique_ptr<ITab> createTab(const ParentPtr<ITabHost>&) const;

    private:
        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator<(const TabRegistryEntry&, const TabRegistryEntry&);
}

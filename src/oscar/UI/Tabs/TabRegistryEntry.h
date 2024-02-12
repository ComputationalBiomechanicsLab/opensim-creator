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
            std::function<std::unique_ptr<ITab>(ParentPtr<ITabHost> const&)> constructor_
        );
        TabRegistryEntry(TabRegistryEntry const&);
        TabRegistryEntry(TabRegistryEntry&&) noexcept;
        TabRegistryEntry& operator=(TabRegistryEntry const&);
        TabRegistryEntry& operator=(TabRegistryEntry&&) noexcept;
        ~TabRegistryEntry() noexcept;

        CStringView getName() const;
        std::unique_ptr<ITab> createTab(ParentPtr<ITabHost> const&) const;

    private:
        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator<(TabRegistryEntry const&, TabRegistryEntry const&);
}

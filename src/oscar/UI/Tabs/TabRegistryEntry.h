#pragma once

#include <oscar/Utils/CStringView.h>

#include <functional>
#include <memory>

namespace osc { class Tab; }
namespace osc { class Widget; }

namespace osc
{
    // reference-counted definition for an available tab
    class TabRegistryEntry final {
    public:
        TabRegistryEntry(
            CStringView name,
            std::function<std::unique_ptr<Tab>(Widget&)> tab_constructor
        );

        CStringView name() const;
        std::unique_ptr<Tab> construct_tab(Widget&) const;

    private:
        class Impl;
        std::shared_ptr<Impl> impl_;
    };
}

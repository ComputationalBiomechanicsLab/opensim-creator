#pragma once

#include <liboscar/utilities/c_string_view.h>

#include <functional>
#include <memory>

namespace osc
{
    class Tab;
    class Widget;

    // reference-counted definition for an available tab
    class TabRegistryEntry final {
    public:
        explicit TabRegistryEntry(
            CStringView name, std::function<std::unique_ptr<Tab>(Widget*)> tab_constructor);

        CStringView name() const;
        std::unique_ptr<Tab> construct_tab(Widget*) const;

    private:
        class Impl;
        std::shared_ptr<Impl> impl_;
    };
}

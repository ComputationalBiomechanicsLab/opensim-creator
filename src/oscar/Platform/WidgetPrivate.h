#pragma once

#include <oscar/Utils/SharedLifetimeBlock.h>

namespace osc
{
    class WidgetPrivate {
    public:
        virtual ~WidgetPrivate() noexcept = default;

        SharedLifetimeBlock& lifetime() { return lifetime_; }
    private:
        SharedLifetimeBlock lifetime_;
    };
}

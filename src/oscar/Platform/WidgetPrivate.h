#pragma once

#include <oscar/Platform/Widget.h>

#include <oscar/Utils/SharedLifetimeBlock.h>

namespace osc
{
    class WidgetPrivate {
    public:
        virtual ~WidgetPrivate() noexcept = default;

        SharedLifetimeBlock& lifetime() { return lifetime_; }

        Widget& owner() { return *owner_; }
        const Widget& owner() const { return *owner_; }

    protected:
        explicit WidgetPrivate(Widget& owner) : owner_{&owner} {}

    private:
        Widget* owner_;
        SharedLifetimeBlock lifetime_;
    };
}

#pragma once

#include <oscar/Platform/Widget.h>
#include <oscar/Utils/SharedLifetimeBlock.h>

#define OSC_OWNER_GETTERS(OwnerClass)                                                             \
    const OwnerClass& owner() const { return reinterpret_cast<const OwnerClass&>(base_owner()); } \
    OwnerClass& owner() { return reinterpret_cast<OwnerClass&>(base_owner()); }

namespace osc
{
    class WidgetPrivate {
    public:
        explicit WidgetPrivate(Widget& owner) : owner_{&owner} {}
        virtual ~WidgetPrivate() noexcept = default;

        SharedLifetimeBlock& lifetime() { return lifetime_; }

    protected:
        Widget& base_owner() { return *owner_; }
        const Widget& base_owner() const { return *owner_; }

        OSC_OWNER_GETTERS(Widget);

    private:
        Widget* owner_;
        SharedLifetimeBlock lifetime_;
    };
}

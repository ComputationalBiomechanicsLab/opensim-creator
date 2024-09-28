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
        explicit WidgetPrivate(Widget& owner, Widget* parent) :
            owner_{&owner},
            parent_{parent ? parent->weak_ref() : nullptr}
        {}
        virtual ~WidgetPrivate() noexcept = default;

        SharedLifetimeBlock& lifetime() { return lifetime_; }

        Widget* parent() { return parent_.get(); }
        const Widget* parent() const { return parent_.get(); }
    protected:
        Widget& base_owner() { return *owner_; }
        const Widget& base_owner() const { return *owner_; }

        OSC_OWNER_GETTERS(Widget);

    private:
        Widget* owner_;
        LifetimedPtr<Widget> parent_;
        SharedLifetimeBlock lifetime_;
    };
}

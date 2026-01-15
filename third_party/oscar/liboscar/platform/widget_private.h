#pragma once

#include <liboscar/platform/widget.h>
#include <liboscar/utils/c_string_view.h>
#include <liboscar/utils/shared_lifetime_block.h>

#include <string_view>

#define OSC_OWNER_GETTERS(OwnerClass)                                                             \
    const OwnerClass& owner() const { return reinterpret_cast<const OwnerClass&>(base_owner()); } \
    OwnerClass& owner() { return reinterpret_cast<OwnerClass&>(base_owner()); }

namespace osc
{
    class WidgetPrivate {
    public:
        explicit WidgetPrivate(Widget& owner, Widget* parent) :
            owner_{&owner},
            parent_{parent}
        {}
        virtual ~WidgetPrivate() noexcept = default;

        LifetimeWatcher lifetime_watcher() const { return lifetime_.watch(); }

        Widget* parent() { return parent_; }
        const Widget* parent() const { return parent_; }

        void set_parent(Widget* new_parent) { parent_ = new_parent; }

        CStringView name() const { return name_; }
        void set_name(std::string_view name) { name_ = name; }

    protected:
        Widget& base_owner() { return *owner_; }
        const Widget& base_owner() const { return *owner_; }

        OSC_OWNER_GETTERS(Widget);

    private:
        Widget* owner_;
        Widget* parent_;
        SharedLifetimeBlock lifetime_;
        std::string name_;
    };
}

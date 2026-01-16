#pragma once

#include <liboscar/platform/widget_private.h>
#include <liboscar/ui/tabs/tab.h>
#include <liboscar/utils/uid.h>

#include <string_view>

namespace osc
{
    class TabPrivate : public WidgetPrivate {
    public:
        explicit TabPrivate(Tab& owner, Widget* parent, std::string_view name) :
            WidgetPrivate{owner, parent}
        {
            set_name(name);
        }

        UID id() const { return id_; }
    private:
        UID id_;
    };
}

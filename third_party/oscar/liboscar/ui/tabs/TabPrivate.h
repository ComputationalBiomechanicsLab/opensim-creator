#pragma once

#include <liboscar/platform/WidgetPrivate.h>
#include <liboscar/ui/tabs/Tab.h>
#include <liboscar/utils/UID.h>

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

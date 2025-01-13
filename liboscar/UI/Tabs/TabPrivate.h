#pragma once

#include <liboscar/Platform/WidgetPrivate.h>
#include <liboscar/UI/Tabs/Tab.h>
#include <liboscar/Utils/UID.h>

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

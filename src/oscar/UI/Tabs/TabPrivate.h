#pragma once

#include <oscar/Platform/WidgetPrivate.h>
#include <oscar/UI/Tabs/Tab.h>
#include <oscar/Utils/UID.h>

#include <string>
#include <string_view>

namespace osc
{
    class TabPrivate : public WidgetPrivate {
    public:
        explicit TabPrivate(Tab& owner, std::string_view tab_name) :
            WidgetPrivate{owner},
            name_{tab_name}
        {}

        UID id() const { return id_; }
        CStringView name() const { return name_; }
        void set_name(std::string new_name) { name_ = std::move(new_name); }
    private:
        UID id_;
        std::string name_;
    };
}

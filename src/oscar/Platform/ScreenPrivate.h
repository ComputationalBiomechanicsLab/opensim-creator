#pragma once

#include <oscar/Platform/Screen.h>

#include <oscar/Platform/WidgetPrivate.h>

namespace osc
{
    class ScreenPrivate : public WidgetPrivate {
    public:
        explicit ScreenPrivate(Screen& owner, Widget* parent, std::string_view name) :
            WidgetPrivate{owner, parent}
        {
            set_name(name);
        }
    protected:
        OSC_OWNER_GETTERS(Screen);

    private:
        friend class Screen;
    };
}

#pragma once

#include <oscar/Platform/Screen.h>

#include <oscar/Platform/WidgetPrivate.h>

namespace osc
{
    class ScreenPrivate : public WidgetPrivate {
    public:
        explicit ScreenPrivate(Screen& owner) :
            WidgetPrivate{owner}
        {}
    protected:
        OSC_OWNER_GETTERS(Screen);

    private:
        friend class Screen;
    };
}

#pragma once

#include <oscar/Platform/Widget.h>
#include <oscar/Utils/CStringView.h>

#include <memory>

namespace osc { class ScreenPrivate; }

namespace osc
{
    // virtual interface to a top-level screen shown by the application
    //
    // the application shows exactly one top-level `Screen` to the user at
    // any given time
    class Screen : public Widget {
    protected:
        explicit Screen();
        explicit Screen(std::unique_ptr<ScreenPrivate>&&);

        OSC_WIDGET_DATA_GETTERS(ScreenPrivate);
    };
}

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
    public:
        CStringView name() const { return impl_get_name(); }
        void on_mount() { impl_on_mount(); }
        void on_unmount() { impl_on_unmount(); }
        void on_tick() { impl_on_tick(); }
        void on_draw() { impl_on_draw(); }

    protected:
        explicit Screen();
        explicit Screen(std::unique_ptr<ScreenPrivate>&&);

        OSC_WIDGET_DATA_GETTERS(ScreenPrivate);

    private:
        // returns the name of the screen
        virtual CStringView impl_get_name() const;

        // called before the app is about to start pump-/tick-/draw-ing the screen
        virtual void impl_on_mount() {}

        // called after the last time the app pumps/ticks/draws the screen
        virtual void impl_on_unmount() {}

        // called by app once per frame (float is a timedelta in seconds)
        virtual void impl_on_tick() {}

        // called by app when the screen should render into the current framebuffer
        virtual void impl_on_draw() = 0;
    };
}

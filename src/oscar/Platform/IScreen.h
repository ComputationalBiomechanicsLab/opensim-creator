#pragma once

#include <oscar/Utils/CStringView.h>

#include <typeinfo>

namespace osc { class Event; }

namespace osc
{
    // virtual interface to a top-level screen shown by the application
    //
    // the application shows exactly one top-level `Screen` to the user at
    // any given time
    class IScreen {
    protected:
        IScreen() = default;
        IScreen(const IScreen&) = default;
        IScreen(IScreen&&) noexcept = default;
        IScreen& operator=(const IScreen&) = default;
        IScreen& operator=(IScreen&&) noexcept = default;
    public:
        virtual ~IScreen() noexcept = default;

        CStringView name() const { return impl_get_name(); }
        void on_mount() { impl_on_mount(); }
        void on_unmount() { impl_on_unmount(); }
        bool on_event(Event& e) { return impl_on_event(e); }
        void on_tick() { impl_on_tick(); }
        void on_draw() { impl_on_draw(); }

    private:
        // returns the name of the screen (handy for debugging/logging)
        virtual CStringView impl_get_name() const
        {
            const IScreen& s = *this;
            return typeid(s).name();
        }

        // called before the app is about to start pump-/tick-/draw-ing the screen
        virtual void impl_on_mount() {}

        // called after the last time the app pumps/ticks/draws the screen
        virtual void impl_on_unmount() {}

        // called by app to pump an event to the screen
        virtual bool impl_on_event(Event&) { return false; }

        // called by app once per frame (float is a timedelta in seconds)
        virtual void impl_on_tick() {}

        // called by app when the screen should render into the current framebuffer
        virtual void impl_on_draw() = 0;
    };
}

#pragma once

#include <oscar/Utils/CStringView.h>

#include <SDL_events.h>

#include <typeinfo>

namespace osc
{
    // virtual interface to a top-level screen shown by the application
    //
    // the application shows exactly one top-level `Screen` to the user at
    // any given time
    class IScreen {
    protected:
        IScreen() = default;
        IScreen(IScreen const&) = default;
        IScreen(IScreen&&) noexcept = default;
        IScreen& operator=(IScreen const&) = default;
        IScreen& operator=(IScreen&&) noexcept = default;
    public:
        virtual ~IScreen() noexcept = default;

        CStringView getName() const { return implGetName(); }
        void onMount() { implOnMount(); }
        void onUnmount() { implOnUnmount(); }
        void onEvent(SDL_Event const& e) { implOnEvent(e); }
        void onTick() { implOnTick(); }
        void onDraw() { implOnDraw(); }

    private:

        // returns the name of the screen (handy for debugging/logging)
        virtual CStringView implGetName() const
        {
            IScreen const& s = *this;
            return typeid(s).name();
        }

        // called before the app is about to start pump-/tick-/draw-ing the screen
        virtual void implOnMount() {}

        // called after the last time the app pumps/ticks/draws the screen
        virtual void implOnUnmount() {}

        // called by app to pump an event to the screen
        virtual void implOnEvent(SDL_Event const&) {}

        // called by app once per frame (float is a timedelta in seconds)
        virtual void implOnTick() {}

        // called by app when the screen should render into the current framebuffer
        virtual void implOnDraw() = 0;
    };
}

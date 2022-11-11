#pragma once

#include <SDL_events.h>

#include <typeinfo>

namespace osc
{
    // top-level screen shown by the application
    //
    // the application shows exactly one top-level `Screen` to the user at
    // any given time
    class Screen {
    protected:
        Screen() = default;
        Screen(Screen const&) = default;
        Screen(Screen&&) noexcept = default;
        Screen& operator=(Screen const&) = default;
        Screen& operator=(Screen&&) noexcept = default;
    public:
        virtual ~Screen() noexcept = default;

        char const* name() const
        {
            return implName();
        }

        void onMount()
        {
            implOnMount();
        }

        void onUnmount()
        {
            implOnUnmount();
        }

        void onEvent(SDL_Event const& e)
        {
            implOnEvent(e);
        }

        void onTick()
        {
            implOnTick();
        }

        void onDraw()
        {
            implOnDraw();
        }

    private:

        // returns the name of the screen (handy for debugging/logging)
        virtual char const* implName() const
        {
            Screen const& s = *this;
            return typeid(s).name();
        }

        // called before the app is about to start pump-/tick-/draw-ing the screen
        virtual void implOnMount()
        {
        }

        // called after the last time the app pumps/ticks/draws the screen
        virtual void implOnUnmount()
        {
        }

        // called by app to pump an event to the screen
        virtual void implOnEvent(SDL_Event const&)
        {
        }

        // called by app once per frame (float is a timedelta in seconds)
        virtual void implOnTick()
        {
        }

        // called by app when the screen should render into the current framebuffer
        virtual void implOnDraw() = 0;
    };
}

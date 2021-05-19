#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

#include <stdexcept>

namespace osc {

    // A plain screen for showing an error message + log to the user
    //
    // this is typically the screen the top-level Application automatically
    // transitions into if an exception bubbles all the way to the top of the
    // main draw loop. It's the best it can do: tell the user as much as possible
    class Error_screen final : public Screen {
        struct Impl;
        Impl* impl;

    public:
        Error_screen(std::exception const& ex);
        Error_screen(Error_screen const&) = delete;
        Error_screen(Error_screen&&) = delete;
        Error_screen& operator=(Error_screen const&) = delete;
        Error_screen& operator=(Error_screen&&) = delete;
        ~Error_screen() noexcept override;

        bool on_event(SDL_Event const&) override;
        void draw() override;
    };
}

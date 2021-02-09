#pragma once

#include "screen.hpp"

#include <SDL_events.h>
#include <stdexcept>

// blank screen that shows the exception message in-UI
namespace osmv {
    struct Error_screen_impl;
    class Error_screen final : public Screen {
        Error_screen_impl* impl;
    public:
        Error_screen(std::exception const& ex);
        Error_screen(Error_screen const&) = delete;
        Error_screen(Error_screen&&) = delete;
        Error_screen& operator=(Error_screen const&) = delete;
        Error_screen& operator=(Error_screen&&) = delete;
        ~Error_screen() noexcept;

        bool on_event(SDL_Event const&) override;
        void draw() override;
    };
}

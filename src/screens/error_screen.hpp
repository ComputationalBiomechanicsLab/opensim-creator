#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

#include <stdexcept>

// blank screen that shows the exception message in-UI
namespace osmv {
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

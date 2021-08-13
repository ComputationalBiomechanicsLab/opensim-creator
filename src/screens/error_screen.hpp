#pragma once

#include "src/screen.hpp"

#include <stdexcept>
#include <memory>

namespace osc {

    // A plain screen for showing an error message + log to the user
    //
    // this is typically the screen the top-level Application automatically
    // transitions into if an exception bubbles all the way to the top of the
    // main draw loop. It's the best it can do: tell the user as much as possible
    class Error_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        // create an error screen that shows an exception message
        Error_screen(std::exception const&);
        ~Error_screen() noexcept override;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void draw() override;
    };
}

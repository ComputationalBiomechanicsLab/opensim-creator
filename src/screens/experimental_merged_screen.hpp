#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>
#include <memory>

namespace OpenSim {
    class Model;
}

// show model screen: main UI screen that shows a loaded OpenSim
// model /w UX, manipulators, etc.
namespace osmv {
    class Experimental_merged_screen final : public Screen {
        struct Impl;
        Impl* impl;

    public:
        Experimental_merged_screen();
        Experimental_merged_screen(std::unique_ptr<OpenSim::Model>);
        Experimental_merged_screen(Experimental_merged_screen const&) = delete;
        Experimental_merged_screen(Experimental_merged_screen&&) = delete;
        Experimental_merged_screen& operator=(Experimental_merged_screen const&) = delete;
        Experimental_merged_screen& operator=(Experimental_merged_screen&&) = delete;
        ~Experimental_merged_screen() noexcept override;

        bool on_event(SDL_Event const&) override;
        void tick() override;
        void draw() override;
    };
}

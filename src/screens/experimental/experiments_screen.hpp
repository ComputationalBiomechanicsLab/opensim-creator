#pragma once

#include "src/screen.hpp"

#include <memory>

namespace osc {
    
    // top-level "experiments" screen
    //
    // for development and featuretest use. This is where new functionality etc.
    // that isn't quite ready for the main UI gets dumped
    class Experiments_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        Experiments_screen();
        ~Experiments_screen() noexcept override;

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void draw() override;
    };
}

#pragma once

#include "src/screens/screen.hpp"

#include <memory>

namespace osc {
    struct Main_editor_state;
}

namespace osc {

    // top-level splash screen
    //
    // this is shown when OSC boots and contains a list of previously opened files, etc.
    class Splash_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        // creates a fresh main editor state
        Splash_screen();

        // recycles an existing main editor state (so the user's tabs etc. persist)
        Splash_screen(std::shared_ptr<Main_editor_state>);

        ~Splash_screen() noexcept override;

        void tick(float) override;
        void draw() override;
    };
}

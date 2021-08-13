#pragma once

#include "src/screens/screen.hpp"

#include <SDL_events.h>

#include <filesystem>
#include <memory>

namespace osc {
    struct Main_editor_state;
}

namespace osc {

    // shows a basic loading message while an .osim file loads
    class Loading_screen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> impl;

    public:
        // load the supplied path (assumed to be an .osim) and then transition
        // to the editor screen
        Loading_screen(std::shared_ptr<Main_editor_state>, std::filesystem::path);
        ~Loading_screen() noexcept override;

        bool on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}

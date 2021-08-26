#pragma once

#include "src/screen.hpp"
#include "src/main_editor_state.hpp"

#include <SDL_events.h>

#include <filesystem>
#include <memory>

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

        void on_mount() override;
        void on_unmount() override;
        void on_event(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}

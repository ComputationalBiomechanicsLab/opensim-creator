#pragma once

#include "opensim_wrapper.hpp"
#include "screen.hpp"

#include <filesystem>
#include <memory>

// show model screen: main UI screen that shows a loaded OpenSim model /w UX, manipulators, etc.
namespace osmv {
    struct Show_model_screen_impl;

    class Show_model_screen final : public Screen {
        std::unique_ptr<Show_model_screen_impl> impl;

    public:
        Show_model_screen(Application& app, std::filesystem::path, osmv::Model);
        Show_model_screen(Show_model_screen const&) = delete;
        Show_model_screen(Show_model_screen&&) = delete;
        Show_model_screen& operator=(Show_model_screen const&) = delete;
        Show_model_screen& operator=(Show_model_screen&&) = delete;
        ~Show_model_screen() noexcept override;

        Event_response on_event(SDL_Event const&) override;
        void tick() override;
        void draw() override;
    };
}

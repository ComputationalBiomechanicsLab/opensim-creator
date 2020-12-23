#pragma once

#include "screen.hpp"
#include "opensim_wrapper.hpp"

#include <string>
#include <memory>

// show_model_screen: main UI screen for showing an OpenSim model

namespace osmv {
    struct Show_model_screen_impl;

    class Show_model_screen final : public Screen {
        std::unique_ptr<Show_model_screen_impl> impl;
    public:
        Show_model_screen(std::string path, osim::OSMV_Model model);
        Show_model_screen(Show_model_screen const&) = delete;
        Show_model_screen(Show_model_screen&&) = delete;
        Show_model_screen& operator=(Show_model_screen const&) = delete;
        Show_model_screen& operator=(Show_model_screen&&) = delete;
        ~Show_model_screen() noexcept override;

        Screen_response tick(Application&) override;
        Screen_response handle_event(Application&, SDL_Event&) override;

        void draw(Application&) override;
    };
}

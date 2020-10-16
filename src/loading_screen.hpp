#pragma once

#include "opensim_wrapper.hpp"
#include "screen.hpp"

#include <string>
#include <future>
#include <vector>

namespace osmv {
    class Loading_screen final : public Screen {
        std::string path;
        std::future<std::vector<osim::Geometry>> result;
    public:
        Loading_screen(std::string _path);

        virtual void init(Application&) override;
        Screen_response handle_event(Application&, SDL_Event&) override;
        void draw(Application&) override;
    };
}

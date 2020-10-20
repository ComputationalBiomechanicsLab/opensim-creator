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

        Screen_response tick(Application&) override;
        void draw(Application&) override;
    };
}

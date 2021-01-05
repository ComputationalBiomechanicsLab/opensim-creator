#pragma once

#include "opensim_wrapper.hpp"
#include "screen.hpp"

#include <string>
#include <future>
#include <vector>

namespace osmv {
    // loading screen: screen shown when UI has just booted and is loading (e.g.) an osim file
    class Loading_screen final : public Screen {
        std::string path;
        std::future<osmv::Model> result;
    public:
        Loading_screen(std::string _path);

        Screen_response tick(Application&) override;
        void draw(Application&) override;
    };
}

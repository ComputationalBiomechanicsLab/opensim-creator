#pragma once

#include "screen.hpp"

namespace osmv {
    struct Loading_screen_impl;

    // loading screen: screen shown when UI has just booted and is loading (e.g.) an osim file
    class Loading_screen final : public Screen {
        Loading_screen_impl* impl;
    public:
        Loading_screen(char const* _path);
        Loading_screen(Loading_screen const&) = delete;
        Loading_screen& operator=(Loading_screen const&) = delete;
        Loading_screen& operator=(Loading_screen&&) = delete;
        ~Loading_screen() noexcept;

        Screen_response tick(Application&) override;
        void draw(Application&) override;
    };
}

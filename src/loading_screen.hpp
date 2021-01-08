#pragma once

#include "screen.hpp"

#include <memory>
#include <filesystem>

namespace osmv {
    struct Loading_screen_impl;

    // loading screen: screen shown when UI has just booted and is loading (e.g.) an osim file
    class Loading_screen final : public Screen {
        std::unique_ptr<Loading_screen_impl> impl;
    public:
        Loading_screen(Application&, std::filesystem::path const&);
        Loading_screen(Loading_screen const&) = delete;
        Loading_screen& operator=(Loading_screen const&) = delete;
        Loading_screen& operator=(Loading_screen&&) = delete;
        ~Loading_screen() noexcept override;

        Screen_response tick(Application&) override;
        void draw(Application&) override;
    };
}

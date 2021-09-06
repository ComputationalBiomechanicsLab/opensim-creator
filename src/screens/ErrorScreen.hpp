#pragma once

#include "src/Screen.hpp"

#include <stdexcept>
#include <memory>

namespace osc {

    // A plain screen for showing an error message + log to the user
    //
    // this is typically the screen the top-level Application automatically
    // transitions into if an exception bubbles all the way to the top of the
    // main draw loop. It's the best it can do: tell the user as much as possible
    class ErrorScreen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        // create an error screen that shows an exception message
        ErrorScreen(std::exception const&);
        ~ErrorScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void draw() override;
    };
}

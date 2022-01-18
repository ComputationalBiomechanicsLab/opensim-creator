#pragma once

#include "src/Screen.hpp"

#include <stdexcept>
#include <memory>

namespace osc
{
    // A plain screen for showing an error message + log to the user
    class ErrorScreen final : public Screen {
    public:
        // create an error screen that shows an exception message
        ErrorScreen(std::exception const&);

        ~ErrorScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void draw() override;

        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;
    };
}

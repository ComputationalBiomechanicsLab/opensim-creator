#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

#include <stdexcept>

namespace osc
{
    // A plain screen for showing an error message + log to the user
    class ErrorScreen final : public Screen {
    public:

        // create an error screen that shows an exception message
        explicit ErrorScreen(std::exception const&);
        ErrorScreen(ErrorScreen const&) = delete;
        ErrorScreen(ErrorScreen&&) noexcept;
        ErrorScreen& operator=(ErrorScreen const&) = delete;
        ErrorScreen& operator=(ErrorScreen&&) noexcept;
        ~ErrorScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void draw() override;

        class Impl;
    private:
        Impl* m_Impl;
    };
}

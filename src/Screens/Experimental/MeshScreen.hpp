#pragma once

#include "src/Screen.hpp"

#include <SDL_events.h>

#include <memory>

namespace osc {

    // META: this is a valid screen with `MeshScreen` as a replaceable
    //       string that users can "Find+Replace" to make their own screen impl

    class MeshScreen final : public Screen {
    public:
        struct Impl;
    private:
        std::unique_ptr<Impl> m_Impl;

    public:
        MeshScreen();
        ~MeshScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void tick(float) override;
        void draw() override;
    };
}

#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // screen that displays a mesh (to confirm the `Mesh` abstraction works)
    class MeshScreen final : public Screen {
    public:
        MeshScreen();
        MeshScreen(MeshScreen const&) = delete;
        MeshScreen(MeshScreen&&) noexcept;
        MeshScreen& operator=(MeshScreen const&) = delete;
        MeshScreen& operator=(MeshScreen&&) noexcept;
        ~MeshScreen() noexcept override;

        void onMount() override;
        void onUnmount() override;
        void onEvent(SDL_Event const&) override;
        void onTick() override;
        void onDraw() override;

    private:
        class Impl;
        Impl* m_Impl;
    };
}

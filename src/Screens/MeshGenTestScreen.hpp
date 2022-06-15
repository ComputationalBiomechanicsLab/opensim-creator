#pragma once

#include "src/Platform/Screen.hpp"

#include <SDL_events.h>

namespace osc
{
    // basic screen for personal math experiments
    class MeshGenTestScreen final : public Screen {
    public:
        MeshGenTestScreen();
        MeshGenTestScreen(MeshGenTestScreen const&) = delete;
        MeshGenTestScreen(MeshGenTestScreen&&) noexcept;
        MeshGenTestScreen& operator=(MeshGenTestScreen const&) = delete;
        MeshGenTestScreen& operator=(MeshGenTestScreen&&) noexcept;
        ~MeshGenTestScreen() noexcept override;

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
#pragma once

#include <glm/vec2.hpp>
#include <SDL_events.h>

#include <array>
#include <cstdint>

namespace osc {

    struct IoPoller final {

        // drawable size of display
        glm::vec2 DisplaySize;

        // seconds since last update
        float DeltaTime;

        // current mouse position, in pixels, relative to top-left corner of screen
        glm::vec2 MousePos;

        // previous mouse position
        glm::vec2 MousePosPrevious;

        // mouse position delta from previous update (== MousePos - MousePosPrevious)
        glm::vec2 MouseDelta;

        // indicates that the backend should set the OS mouse pos
        //
        // next frame, the backend will warp to the MousePosWarpTo location, but
        // will ensure that MousePosDelta behaves "as if" the user moved their
        // mouse from MousePosPrevious to MousePosWarpTo
        //
        // the backend will reset WantMousePosWarpTo to `false` after
        // performing the warp
        bool WantMousePosWarpTo;
        glm::vec2 MousePosWarpTo;

        // mouse button states (0: left, 1: right, 2: middle)
        std::array<bool, 3> MousePressed;

        // keyboard keys that are currently pressed
        std::array<bool, 512> KeysDown;
        bool KeyShift;
        bool KeyCtrl;
        bool KeyAlt;

        // duration, in seconds, that each key has been pressed for
        //
        // == -1.0f if key is not down this frame
        // ==  0.0f if the key was pressed this frame
        // >   0.0f if the key was pressed in a previous frame
        std::array<float, 512> KeysDownDuration;

        // as above, but the *previous* frame's values
        //
        // the utility of this is for detecting when a key was released.
        // e.g. if a value in here is >= 0.0f && !KeysDown[key] then the
        // key must have been released this frame
        std::array<float, 512> KeysDownDurationPrev;

        // private members updated by `onEvent` to ensure the poller doesn't
        // miss things that happen too quickly
        bool _mousePressedEvents[3];

        // construct IoPoller: assumes `App` has been initialized
        IoPoller();

        // feed an event into `IoPoller`, which may update some internal datastructures
        void onEvent(SDL_Event const&);

        // update the `IoPoller`: should be called once per frame
        void onUpdate();
    };
}

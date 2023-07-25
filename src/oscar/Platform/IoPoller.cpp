#include "IoPoller.hpp"

#include "oscar/Platform/App.hpp"

#include <glm/vec2.hpp>
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include <algorithm>
#include <cstddef>

namespace
{
    template<typename T, size_t N>
    std::array<T, N> ArrayFilledWith(T v)
    {
        std::array<T, N> rv{};
        std::fill(rv.begin(), rv.end(), v);
        return rv;
    }
}

osc::IoPoller::IoPoller() :
    DisplaySize{-1.0f, -1.0f},
    DeltaTime{0.0f},
    MousePos{0.0f, 0.0f},
    MousePosPrevious{0.0f, 0.0f},
    MouseDelta{0.0f, 0.0f},
    WantMousePosWarpTo{false},
    MousePosWarpTo{-1.0f, -1.0f},
    MousePressed{false, false, false},
    KeysDown{ArrayFilledWith<bool, 512>(false)},
    KeyShift{false},
    KeyCtrl{false},
    KeyAlt{false},
    KeysDownDuration{ArrayFilledWith<float, 512>(-1.0f)},
    KeysDownDurationPrev{ArrayFilledWith<float, 512>(-1.0f)},
    _mousePressedEvents{false, false, false}
{
}

// feed an event into `IoPoller`, which may update some internal datastructures
void osc::IoPoller::onEvent(SDL_Event const& e)
{
    if (e.type == SDL_MOUSEBUTTONDOWN)
    {
        switch (e.button.button) {
        case SDL_BUTTON_LEFT: _mousePressedEvents[0] = true; break;
        case SDL_BUTTON_RIGHT: _mousePressedEvents[1] = true; break;
        case SDL_BUTTON_MIDDLE: _mousePressedEvents[2] = true; break;
        }
    }
    else if (e.type == SDL_KEYUP || e.type == SDL_KEYDOWN)
    {
        KeysDown[e.key.keysym.scancode] = e.type == SDL_KEYDOWN;
        App const& app = App::get();
        KeyShift = app.isShiftPressed();
        KeyCtrl = app.isCtrlPressed();
        KeyAlt = app.isAltPressed();
    }
}

// update the `IoPoller`: should be called once per frame
void osc::IoPoller::onUpdate()
{
    App const& app = App::get();

    DisplaySize = app.dims();
    DeltaTime = static_cast<float>(app.getFrameDeltaSinceLastFrame().count());

    // MousePos, MousePosPrevious, MousePosDelta, MousePressed
    auto mouseState = app.getMouseState();
    MousePressed[0] = _mousePressedEvents[0] || mouseState.LeftDown;
    MousePressed[1] = _mousePressedEvents[1] || mouseState.RightDown;
    MousePressed[2] = _mousePressedEvents[2] || mouseState.MiddleDown;
    MousePosPrevious = MousePos;
    MousePos = mouseState.pos;
    MouseDelta = glm::vec2{glm::ivec2{MousePos - MousePosPrevious}};

    // (edge-case)
    //
    // if the caller wants to set the mouse position, then it should be
    // set. However, to ensure that Delta == Cur-Prev, we need to create
    // a "fake"  *prev* that behaves "as if" the mouse moved from some
    // location to the warp location
    if (WantMousePosWarpTo && app.isWindowFocused())
    {
        App::upd().warpMouseInWindow(MousePosWarpTo);
        MousePos = MousePosWarpTo;
        MousePosPrevious = MousePosWarpTo - MouseDelta;
        WantMousePosWarpTo = false;
    }

    // (KeysDown, Shift-/Ctrl-/Alt-Down handled by events)

    // KeysDownDurationPrev
    std::copy(KeysDownDuration.begin(), KeysDownDuration.end(), KeysDownDurationPrev.begin());

    // KeysDownDuration
    for (size_t i = 0; i < KeysDown.size(); ++i)
    {
        if (!KeysDown[i])
        {
            KeysDownDuration[i] = -1.0f;
        }
        else
        {
            KeysDownDuration[i] = KeysDownDuration[i] < 0.0f ? 0.0f : KeysDownDuration[i] + DeltaTime;
        }
    }
}

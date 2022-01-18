#include "IoPoller.hpp"

#include "src/App.hpp"
#include "src/Assertions.hpp"

#include <SDL_events.h>

#include <algorithm>

osc::IoPoller::IoPoller() :
    DisplaySize{-1.0f, -1.0f},
    Ticks{App::cur().getTicks()},
    TickFrequency{App::cur().getTickFrequency()},
    DeltaTime{0.0f},
    MousePos{0.0f, 0.0f},
    MousePosPrevious{0.0f, 0.0f},
    MouseDelta{0.0f, 0.0f},
    WantMousePosWarpTo{false},
    MousePosWarpTo{-1.0f, -1.0f},
    MousePressed{false, false, false},
    // KeysDown: handled below
    KeyShift{false},
    KeyCtrl{false},
    KeyAlt{false},
    // KeysDownDuration: handled below
    // KeysDownDurationPrev: handled below
    _mousePressedEvents{false, false, false}
{
    std::fill(KeysDown.begin(), KeysDown.end(), false);
    std::fill(KeysDownDuration.begin(), KeysDownDuration.end(), -1.0f);
    std::fill(KeysDownDurationPrev.begin(), KeysDownDurationPrev.end(), -1.0f);
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
        KeyShift = App::cur().isShiftPressed();
        KeyCtrl = App::cur().isCtrlPressed();
        KeyAlt = App::cur().isAltPressed();
    }
}

// update the `IoPoller`: should be called once per frame
void osc::IoPoller::onUpdate()
{
    DisplaySize = App::cur().dims();

    // Ticks, (IO ctor: TickFrequency), DeltaTime
    auto curTicks = App::cur().getTicks();
    double dTicks = static_cast<double>(curTicks - Ticks);
    DeltaTime = static_cast<float>(dTicks/static_cast<double>(TickFrequency));
    Ticks = curTicks;

    // MousePos, MousePosPrevious, MousePosDelta, MousePressed
    auto mouseState = App::cur().getMouseState();
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
    if (WantMousePosWarpTo && App::cur().isWindowFocused())
    {
        App::cur().warpMouseInWindow(MousePosWarpTo);
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

#pragma once

#include <glm/vec2.hpp>

namespace osc
{
    // data structure representing the current application mouse state
    struct MouseState final {
        glm::ivec2 pos;
        bool LeftDown;
        bool RightDown;
        bool MiddleDown;
        bool X1Down;
        bool X2Down;
    };
}
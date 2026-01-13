#pragma once

#include <liboscar/platform/CursorShape.h>

namespace osc
{
    class Cursor final {
    public:
        Cursor() = default;
        explicit Cursor(CursorShape shape) : shape_{shape} {}

        CursorShape shape() const { return shape_; }
    private:
        CursorShape shape_ = CursorShape::Arrow;
    };
}

#pragma once

#include <oscar/Graphics/Color.h>

namespace osc
{
    struct OSCColors final {
        static constexpr Color selected() { return Color::yellow(); }
        static constexpr Color hovered() { return selected().with_alpha(0.6f); }
        static constexpr Color disabled() { return Color::half_grey(); }
        static constexpr Color scrub_current() { return Color::yellow().with_alpha(0.6f); }
        static constexpr Color scrub_hovered() { return Color::yellow().with_alpha(0.3f); }
    };
}

#pragma once

#include <oscar/Maths/Rect.h>

namespace osc
{
    // Represents a physical monitor attached to the host computer.
    class Monitor final {
    public:
        explicit Monitor(const Rect& bounds, const Rect& usable_bounds, float physical_dpi) :
            bounds_{bounds},
            usable_bounds_{usable_bounds},
            physical_dpi_{physical_dpi}
        {}

        // Returns the desktop area represented by this `Monitor`.
        Rect bounds() const { return bounds_; }

        // Returns the usable desktop area represented by this `Monitor`.
        //
        // This is in the same area as `bounds()`, but with portions reserved by the
        // operating system removed. E.g. on Apple's MacOS, this subtracts the area
        // occupied by the menu bar and dock.
        Rect usable_bounds() const { return usable_bounds_; }

        // Returns the number of physical dots/pixels per inch (density) of this `Monitor`.
        //
        // This function can be unreliable: it depends on what information is provided by
        // the underlying operating system (e.g. see `QScreen::physicalDotsPerInch` from Qt
        // or `SDL_GetDisplayDPI` from `SDL2`).
        float physical_dpi() const { return physical_dpi_; }

    private:
        Rect bounds_;
        Rect usable_bounds_;
        float physical_dpi_;
    };
}

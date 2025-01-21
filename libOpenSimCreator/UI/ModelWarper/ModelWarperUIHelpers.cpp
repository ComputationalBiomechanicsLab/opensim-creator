#include "ModelWarperUIHelpers.h"

#include <libopensimcreator/Documents/ModelWarper/ValidationCheckState.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/Platform/IconCodepoints.h>
#include <liboscar/Utils/EnumHelpers.h>

using namespace osc::mow;

EntryStyling osc::mow::ToStyle(ValidationCheckState s)
{
    static_assert(num_options<ValidationCheckState>() == 3);
    switch (s) {
    case ValidationCheckState::Ok:
        return {.icon = OSC_ICON_CHECK, .color = Color::green()};
    case ValidationCheckState::Warning:
        return {.icon = OSC_ICON_EXCLAMATION, .color = Color::orange()};
    default:
    case ValidationCheckState::Error:
        return {.icon = OSC_ICON_TIMES, .color = Color::red()};
    }
}

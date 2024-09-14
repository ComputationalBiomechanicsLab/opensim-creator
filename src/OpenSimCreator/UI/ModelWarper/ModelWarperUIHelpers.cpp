#include "ModelWarperUIHelpers.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Utils/EnumHelpers.h>

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

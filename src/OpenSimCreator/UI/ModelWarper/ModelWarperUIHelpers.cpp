#include "ModelWarperUIHelpers.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Utils/EnumHelpers.h>

using namespace osc::mow;

EntryStyling osc::mow::ToStyle(ValidationState s)
{
    static_assert(NumOptions<ValidationState>() == 3);
    switch (s) {
    case ValidationState::Ok:
        return {.icon = ICON_FA_CHECK, .color = Color::green()};
    case ValidationState::Warning:
        return {.icon = ICON_FA_EXCLAMATION, .color = Color::orange()};
    default:
    case ValidationState::Error:
        return {.icon = ICON_FA_TIMES, .color = Color::red()};
    }
}

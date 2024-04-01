#include "ModelWarperUIHelpers.h"

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>

#include <IconsFontAwesome5.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Utils/EnumHelpers.h>

using namespace osc::mow;

EntryStyling osc::mow::ToStyle(ValidationCheckState s)
{
    static_assert(num_options<ValidationCheckState>() == 3);
    switch (s) {
    case ValidationCheckState::Ok:
        return {.icon = ICON_FA_CHECK, .color = Color::green()};
    case ValidationCheckState::Warning:
        return {.icon = ICON_FA_EXCLAMATION, .color = Color::orange()};
    default:
    case ValidationCheckState::Error:
        return {.icon = ICON_FA_TIMES, .color = Color::red()};
    }
}

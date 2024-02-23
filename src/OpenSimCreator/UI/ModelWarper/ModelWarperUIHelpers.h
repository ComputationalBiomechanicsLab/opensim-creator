#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ValidationState.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Utils/CStringView.h>

namespace osc::mow
{
    struct EntryStyling final {
        CStringView icon;
        Color color;
    };

    EntryStyling ToStyle(ValidationState s);
}

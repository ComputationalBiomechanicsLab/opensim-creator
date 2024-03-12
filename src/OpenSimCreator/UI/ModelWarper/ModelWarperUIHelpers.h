#pragma once

#include <OpenSimCreator/Documents/ModelWarper/ValidationCheckState.h>

#include <oscar/Graphics/Color.h>
#include <oscar/Utils/CStringView.h>

namespace osc::mow
{
    struct EntryStyling final {
        CStringView icon;
        Color color;
    };

    EntryStyling ToStyle(ValidationCheckState s);
}

#pragma once

#include <libopensimcreator/Documents/ModelWarper/ValidationCheckState.h>

#include <liboscar/Graphics/Color.h>
#include <liboscar/Utils/CStringView.h>

namespace osc::mow
{
    struct EntryStyling final {
        CStringView icon;
        Color color;
    };

    EntryStyling ToStyle(ValidationCheckState s);
}

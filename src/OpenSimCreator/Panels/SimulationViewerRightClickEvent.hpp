#pragma once

#include <optional>
#include <string>

namespace osc
{
    struct SimulationViewerRightClickEvent final {
        std::optional<std::string> maybeComponentAbsPath;
    };
}

#pragma once

namespace OpenSim { class Component; }

#include <glm/vec3.hpp>

namespace osc
{
    struct UiModelViewerResponse final {
        OpenSim::Component const* hovertestResult = nullptr;
        bool isMousedOver = false;
        glm::vec3 mouse3DLocation = {0.0f, 0.0f, 0.0f};
    };
}
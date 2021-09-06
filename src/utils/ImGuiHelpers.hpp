#pragma once

#include "src/3d/Model.hpp"

#include <glm/vec2.hpp>

namespace osc {
    void UpdatePolarCameraFromImGuiUserInput(glm::vec2 viewportDims, osc::PolarPerspectiveCamera&);
}

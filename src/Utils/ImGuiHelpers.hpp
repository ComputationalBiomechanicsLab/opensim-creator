#pragma once

#include "src/3D/Model.hpp"

#include <glm/vec2.hpp>

namespace osc {
    void UpdatePolarCameraFromImGuiUserInput(glm::vec2 viewportDims, osc::PolarPerspectiveCamera&);
}

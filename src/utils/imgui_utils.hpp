#pragma once

#include "src/3d/model.hpp"

#include <glm/vec2.hpp>

namespace osc {
    void update_camera_from_user_input(glm::vec2 viewport_dims, osc::Polar_perspective_camera&);
}

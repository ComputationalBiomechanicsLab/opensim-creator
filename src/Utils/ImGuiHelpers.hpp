#pragma once

#include "src/3D/Gl.hpp"
#include "src/3D/Model.hpp"

#include <glm/vec2.hpp>

namespace osc {

    // updates a polar comera's rotation, position, etc. based on ImGui input
    void UpdatePolarCameraFromImGuiUserInput(glm::vec2 viewportDims, osc::PolarPerspectiveCamera&);

    // returns the ImGui content region available in screenspace as a `Rect`
    Rect ContentRegionAvailScreenRect();

    // draws a texutre as an ImGui::Image, assumes UV coords of (0.0, 1.0); (1.0, 0.0)
    void DrawTextureAsImGuiImage(gl::Texture2D&, glm::vec2 dims);
}

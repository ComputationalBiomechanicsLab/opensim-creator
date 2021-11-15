#pragma once

#include "src/3D/Gl.hpp"
#include "src/3D/Model.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>
#include <imgui.h>

namespace osc {

    // updates a polar comera's rotation, position, etc. based on ImGui input
    void UpdatePolarCameraFromImGuiUserInput(glm::vec2 viewportDims, osc::PolarPerspectiveCamera&);

    // returns the ImGui content region available in screenspace as a `Rect`
    Rect ContentRegionAvailScreenRect();

    // draws a texutre as an ImGui::Image, assumes UV coords of (0.0, 1.0); (1.0, 0.0)
    void DrawTextureAsImGuiImage(gl::Texture2D&, glm::vec2 dims);

    // returns `true` if any scancode in the provided range is currently presed down
    bool IsAnyKeyDown(nonstd::span<int const>);
    bool IsAnyKeyDown(std::initializer_list<int const>);

    // returns `true` if the user is pressing either:
    //
    // - left Ctrl
    // - right Ctrl
    // - left Super (mac)
    // - right Super (mac)
    bool IsCtrlOrSuperDown();

    // returns `true` if the user is pressing either left- or right-shift
    bool IsShiftDown();

    // returns `true` if the user is pressing either left- or right-alt
    bool IsAltDown();

    // returns `true` if the specified moouse button was released without the user dragging
    bool IsMouseReleasedWithoutDragging(ImGuiMouseButton, float threshold = 5.0f);

    // draws an overlay tooltip with a header and description
    void DrawTooltip(char const* header, char const* description = nullptr);

    // equivalent to `if (ImGui::IsItemHovered()) DrawTooltip(header, description);`
    void DrawTooltipIfItemHovered(char const* header, char const* description = nullptr);

    // draw overlay axes in bottom-right of screenspace rect
    void DrawAlignmentAxesOverlayInBottomRightOf(glm::mat4 const& viewMtx, Rect const& renderRect);
}

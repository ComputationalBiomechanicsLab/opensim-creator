#include "imgui_utils.hpp"

#include "src/3d/model.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <SDL_events.h>

void osc::update_camera_from_user_input(glm::vec2 viewport_dims, osc::Polar_perspective_camera& camera) {
    camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;
    camera.do_znear_zfar_autoscale();

    // update camera
    //
    // At the moment, all camera updates happen via the middle-mouse. The mouse
    // input is designed to mirror Blender fairly closely (because, imho, it has
    // decent UX for this problem space)
    bool left_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool right_down = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    if (left_down || right_down) {
        ImVec2 screendims = viewport_dims;
        float aspect_ratio = screendims.x / screendims.y;
        ImVec2 delta = ImGui::GetMouseDragDelta(left_down ? ImGuiMouseButton_Left : ImGuiMouseButton_Middle, 0.0f);
        ImGui::ResetMouseDragDelta(left_down ? ImGuiMouseButton_Left : ImGuiMouseButton_Middle);

        // relative vectors
        float rdx = delta.x/screendims.x;
        float rdy = delta.y/screendims.y;

        if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
            // shift + middle-mouse: pan
            camera.do_pan(aspect_ratio, {rdx, rdy});
        } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
            // shift + middle-mouse: zoom
            camera.radius *= 1.0f + rdy;
        } else {
            // middle: mouse drag
            camera.do_drag({rdx, rdy});
        }
    }
}

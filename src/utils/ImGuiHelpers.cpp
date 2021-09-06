#include "ImGuiHelpers.hpp"

#include "src/3d/Model.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <SDL_events.h>

void osc::UpdatePolarCameraFromImGuiUserInput(glm::vec2 viewportDims, osc::PolarPerspectiveCamera& camera) {
    camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;
    camera.rescaleZNearAndZFarBasedOnRadius();

    // update camera
    //
    // At the moment, all camera updates happen via the middle-mouse. The mouse
    // input is designed to mirror Blender fairly closely (because, imho, it has
    // decent UX for this problem space)
    bool leftButtonDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool rightButtonDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);

    if (leftButtonDown || rightButtonDown) {
        ImVec2 screendims = viewportDims;
        float aspectRatio = screendims.x / screendims.y;
        ImVec2 delta = ImGui::GetMouseDragDelta(leftButtonDown ? ImGuiMouseButton_Left : ImGuiMouseButton_Middle, 0.0f);
        ImGui::ResetMouseDragDelta(leftButtonDown ? ImGuiMouseButton_Left : ImGuiMouseButton_Middle);

        // relative vectors
        float rdx = delta.x/screendims.x;
        float rdy = delta.y/screendims.y;

        if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
            // shift + middle-mouse: pan
            camera.pan(aspectRatio, {rdx, rdy});
        } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
            // shift + middle-mouse: zoom
            camera.radius *= 1.0f + rdy;
        } else {
            // middle: mouse drag
            camera.drag({rdx, rdy});
        }
    }
}

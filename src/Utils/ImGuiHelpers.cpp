#include "ImGuiHelpers.hpp"

#include "src/3D/Model.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <SDL_events.h>

void osc::UpdatePolarCameraFromImGuiUserInput(glm::vec2 viewportDims, osc::PolarPerspectiveCamera& camera) {
    // handle mousewheel scrolling
    camera.radius *= 1.0f - ImGui::GetIO().MouseWheel/10.0f;
    camera.rescaleZNearAndZFarBasedOnRadius();

    // these camera controls try to be the union of OpenSim and Blender
    //
    // left drag: drags/orbits camera (OpenSim behavior)
    // left drag + L/R SHIFT: pans camera (CUSTOM behavior: can be handy on laptops where right-click + drag sucks)
    // left drag + L/R CTRL: zoom camera (CUSTOM behavior: can be handy on laptops where right-click + drag sucks)
    // middle drag: drags/orbits camera (Blender behavior)
    // middle drag + L/R SHIFT: pans camera (Blender behavior)
    // middle drag + L/R CTRL: zooms camera (Blender behavior)
    // right drag: pans camera (OpenSim behavior)
    //
    // the reason it's like this is to please legacy OpenSim users *and*
    // users who use modelling software like Blender (which is more popular
    // among newer users looking to make new models)

    float aspectRatio = viewportDims.x / viewportDims.y;

    bool leftDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
    bool middleDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Middle);
    ImGuiMouseButton btn = leftDragging ? ImGuiMouseButton_Left : ImGuiMouseButton_Middle;

    if (leftDragging || middleDragging) {
        glm::vec2 delta = ImGui::GetMouseDragDelta(btn, 0.0f);
        ImGui::ResetMouseDragDelta(btn);

        if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
            camera.pan(aspectRatio, delta/viewportDims);
        } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
            camera.radius *= 1.0f + delta.y/viewportDims.y;
        } else {
            camera.drag(delta/viewportDims);
        }

    } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        glm::vec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right, 0.0f);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
        camera.pan(aspectRatio, delta/viewportDims);

    }
}

#include "ImGuiHelpers.hpp"

#include "src/3D/Model.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <SDL_events.h>
#include <iostream>

void osc::UpdatePolarCameraFromImGuiUserInput(glm::vec2 viewportDims, osc::PolarPerspectiveCamera& camera) {
    using osc::operator<<;

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

    glm::vec2 delta = ImGui::GetIO().MouseDelta;

    if (leftDragging || middleDragging) {
        if (ImGui::IsKeyDown(SDL_SCANCODE_LSHIFT) || ImGui::IsKeyDown(SDL_SCANCODE_RSHIFT)) {
            camera.pan(aspectRatio, delta/viewportDims);
        } else if (ImGui::IsKeyDown(SDL_SCANCODE_LCTRL) || ImGui::IsKeyDown(SDL_SCANCODE_RCTRL)) {
            camera.radius *= 1.0f + delta.y/viewportDims.y;
        } else {
            camera.drag(delta/viewportDims);
        }

    } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        camera.pan(aspectRatio, delta/viewportDims);

    }
}

osc::Rect osc::ContentRegionAvailScreenRect()
{
    glm::vec2 topLeft = ImGui::GetCursorScreenPos();
    glm::vec2 dims = ImGui::GetContentRegionAvail();
    glm::vec2 bottomRight = topLeft + dims;

    return Rect{topLeft, bottomRight};
}

void osc::DrawTextureAsImGuiImage(gl::Texture2D& t, glm::vec2 dims)
{
    void* textureHandle = reinterpret_cast<void*>(static_cast<uintptr_t>(t.get()));
    ImVec2 uv0{0.0f, 1.0f};
    ImVec2 uv1{1.0f, 0.0f};
    ImGui::Image(textureHandle, dims, uv0, uv1);
}

bool osc::IsAnyKeyDown(nonstd::span<int const> keys)
{
    for (auto key : keys) {
        if (ImGui::IsKeyDown(key)) {
            return true;
        }
    }
    return false;
}

bool osc::IsAnyKeyDown(std::initializer_list<int const> keys)
{

    return IsAnyKeyDown(nonstd::span<int const>{keys.begin(), keys.end()});
}

bool osc::IsCtrlOrSuperDown()
{
    return IsAnyKeyDown({ SDL_SCANCODE_LCTRL, SDL_SCANCODE_RCTRL, SDL_SCANCODE_LGUI, SDL_SCANCODE_RGUI });
}

bool osc::IsShiftDown()
{
    return IsAnyKeyDown({SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT});
}

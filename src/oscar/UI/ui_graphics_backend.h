#pragma once

#include <oscar/UI/oscimgui.h>

namespace osc { class RenderTexture; }
namespace osc { class Texture2D; }

// ImGui graphics backend that uses osc's graphics API
struct ImDrawData;

namespace osc::ui::graphics_backend
{
    bool Init();
    void Shutdown();
    void NewFrame();
    void RenderDrawData(ImDrawData*);
    ImTextureID AllocateTextureID(Texture2D const&);
    ImTextureID AllocateTextureID(RenderTexture const&);
}

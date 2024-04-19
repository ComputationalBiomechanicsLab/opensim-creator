#pragma once

#include <oscar/UI/oscimgui.h>

namespace osc { class RenderTexture; }
namespace osc { class Texture2D; }

// ImGui graphics backend that uses osc's graphics API
struct ImDrawData;

namespace osc::ui::graphics_backend
{
    bool init();
    void shutdown();
    void on_start_new_frame();
    void render(ImDrawData*);
    ImTextureID allocate_texture_for_current_frame(const Texture2D&);
    ImTextureID allocate_texture_for_current_frame(const RenderTexture&);
}

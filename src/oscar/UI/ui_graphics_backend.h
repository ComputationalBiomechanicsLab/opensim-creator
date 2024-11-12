#pragma once

#include <cstdint>

namespace osc { class RenderTexture; }
namespace osc { class Texture2D; }

// ImGui graphics backend that uses osc's graphics API
struct ImDrawData;

namespace osc::ui::graphics_backend
{
    using InternalTextureID = unsigned long long;

    bool init();
    void shutdown();
    void on_start_new_frame();
    void render(ImDrawData*);
    InternalTextureID allocate_texture_for_current_frame(const Texture2D&);
    InternalTextureID allocate_texture_for_current_frame(const RenderTexture&);
}

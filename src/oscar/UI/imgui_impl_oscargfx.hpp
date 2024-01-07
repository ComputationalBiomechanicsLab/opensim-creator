#pragma once

#include <imgui.h>

namespace osc { class RenderTexture; }
namespace osc { class Texture2D; }

// ImGui graphics backend that uses osc's graphics API
struct ImDrawData;

bool ImGui_ImplOscarGfx_Init();
void ImGui_ImplOscarGfx_Shutdown();
void ImGui_ImplOscarGfx_NewFrame();
void ImGui_ImplOscarGfx_RenderDrawData(ImDrawData*);
ImTextureID ImGui_ImplOscarGfx_AllocateTextureID(osc::Texture2D const&);
ImTextureID ImGui_ImplOscarGfx_AllocateTextureID(osc::RenderTexture const&);

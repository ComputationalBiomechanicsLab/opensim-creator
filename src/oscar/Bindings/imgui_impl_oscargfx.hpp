#pragma once

// ImGui graphics backend that uses osc's graphics API
struct ImDrawData;

bool ImGui_ImplOscarGfx_Init();
void ImGui_ImplOscarGfx_Shutdown();
void ImGui_ImplOscarGfx_NewFrame();
void ImGui_ImplOscarGfx_RenderDrawData(ImDrawData*);

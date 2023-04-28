#pragma once

struct ImDrawData;

// ImGui graphics backend that uses osc's graphics API

bool ImGui_ImplOSC_Init();
void ImGui_ImplOSC_Shutdown();
void ImGui_ImplOSC_NewFrame();
void ImGui_ImplOSC_RenderDrawData(ImDrawData*);

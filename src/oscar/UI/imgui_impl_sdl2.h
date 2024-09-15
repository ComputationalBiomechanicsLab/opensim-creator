// this was refactored from https://github.com/ocornut/imgui/blob/v1.91.1-docking/backends/imgui_impl_sdl2.h

#pragma once

struct SDL_Window;
typedef union SDL_Event SDL_Event;

bool     ImGui_ImplSDL2_Init(SDL_Window*);
void     ImGui_ImplSDL2_Shutdown();
void     ImGui_ImplSDL2_NewFrame();
bool     ImGui_ImplSDL2_ProcessEvent(const SDL_Event* event);

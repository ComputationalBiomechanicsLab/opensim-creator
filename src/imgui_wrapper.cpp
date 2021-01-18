#include "imgui_wrapper.hpp"

#include "examples/imgui_impl_opengl3.h"
#include "examples/imgui_impl_sdl.h"

igx::Context::Context() : handle{ImGui::CreateContext()} {
}

igx::Context::~Context() noexcept {
    ImGui::DestroyContext(handle);
}

igx::SDL2_Context::SDL2_Context(SDL_Window* w, void* gl) {
    ImGui_ImplSDL2_InitForOpenGL(w, gl);
}

igx::SDL2_Context::~SDL2_Context() noexcept {
    ImGui_ImplSDL2_Shutdown();
}

igx::OpenGL3_Context::OpenGL3_Context(char const* version) {
    ImGui_ImplOpenGL3_Init(version);
}

igx::OpenGL3_Context::~OpenGL3_Context() noexcept {
    ImGui_ImplOpenGL3_Shutdown();
}

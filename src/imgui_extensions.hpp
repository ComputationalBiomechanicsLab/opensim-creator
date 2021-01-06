#pragma once

// forward-declare these so that this header isn't dependent on SDL/imgui
struct SDL_Window;
struct ImGuiContext;

namespace igx {
    class Context final {
        ImGuiContext* handle;
    public:
        Context();
        Context(Context const&) = delete;
        Context(Context&&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;
        ~Context() noexcept;
    };

    struct SDL2_Context final {
        SDL2_Context(SDL_Window* w, void* gl);
        SDL2_Context(SDL2_Context const&) = delete;
        SDL2_Context(SDL2_Context&&) = delete;
        SDL2_Context& operator=(SDL2_Context const&) = delete;
        SDL2_Context& operator=(SDL2_Context&&) = delete;
        ~SDL2_Context() noexcept;
    };

    struct OpenGL3_Context final {
        OpenGL3_Context(char const* version);
        OpenGL3_Context(OpenGL3_Context const&) = delete;
        OpenGL3_Context(OpenGL3_Context&&) = delete;
        OpenGL3_Context& operator=(OpenGL3_Context const&) = delete;
        OpenGL3_Context& operator=(OpenGL3_Context&&) = delete;
        ~OpenGL3_Context() noexcept;
    };
};

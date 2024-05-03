#pragma once

#include <oscar/Maths/Vec2.h>
#include <oscar/Utils/CStringView.h>

#include <SDL.h>
#undef main

#include <stdexcept>
#include <string>
#include <utility>

// sdl wrapper: thin C++ wrappers around SDL
//
// Code in here should:
//
//   - Roughly map 1:1 with SDL
//   - Add RAII to types that have destruction methods
//     (e.g. `SDL_DestroyWindow`)
//   - Use exceptions to enforce basic invariants (e.g. CreateWindow should
//     work or throw)
//
// Emphasis is on simplicity, not "abstraction correctness". It is preferred
// to have an API that is simple, rather than robustly encapsulated etc.

namespace osc::sdl
{
    // RAII wrapper for `SDL_Init` and `SDL_Quit`
    //     https://wiki.libsdl.org/SDL_Quit
    class Context final {
    public:
        explicit Context(Uint32 flags)
        {
            if (SDL_Init(flags) != 0) {
                throw std::runtime_error{std::string{"SDL_Init: failed: "} + SDL_GetError()};
            }
        }
        Context(const Context&) = delete;
        Context(Context&&) noexcept = delete;
        Context& operator=(const Context&) = delete;
        Context& operator=(Context&&) noexcept = delete;
        ~Context() noexcept
        {
            SDL_Quit();
        }
    };

    // https://wiki.libsdl.org/SDL_Init
    inline Context Init(Uint32 flags)
    {
        return Context{flags};
    }

    // RAII wrapper around `SDL_Window` that calls `SDL_DestroyWindow` on dtor
    //     https://wiki.libsdl.org/SDL_CreateWindow
    //     https://wiki.libsdl.org/SDL_DestroyWindow
    class Window final {
    public:
        Window(const Window&) = delete;
        Window(Window&& tmp) noexcept :
            window_handle_{std::exchange(tmp.window_handle_, nullptr)}
        {}
        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) noexcept = delete;
        ~Window() noexcept
        {
            if (window_handle_) {
                SDL_DestroyWindow(window_handle_);
            }
        }

        SDL_Window* get() const { return window_handle_; }

        SDL_Window& operator*() const { return *window_handle_; }

    private:
        friend Window CreateWindoww(CStringView title, int x, int y, int w, int h, Uint32 flags);
        Window(SDL_Window * _ptr) :
            window_handle_{_ptr}
        {}

        SDL_Window* window_handle_;
    };

    // RAII'ed version of `SDL_CreateWindow`
    //     https://wiki.libsdl.org/SDL_CreateWindow
    //
    // CreateWindoww is a typo because `CreateWindow` is defined in the
    // preprocessor
    inline Window CreateWindoww(CStringView title, int x, int y, int w, int h, Uint32 flags)
    {
        SDL_Window* const win = SDL_CreateWindow(title.c_str(), x, y, w, h, flags);

        if (win == nullptr) {
            throw std::runtime_error{std::string{"SDL_CreateWindow failed: "} + SDL_GetError()};
        }

        return Window{win};
    }

    // RAII wrapper around `SDL_GLContext` that calls `SDL_GL_DeleteContext` on dtor
    //     https://wiki.libsdl.org/SDL_GL_DeleteContext
    class GLContext final {
    public:
        GLContext(const GLContext&) = delete;
        GLContext(GLContext&& tmp) noexcept :
            context_handle_{std::exchange(tmp.context_handle_, nullptr)}
        {}
        GLContext& operator=(const GLContext&) = delete;
        GLContext& operator=(GLContext&&) = delete;
        ~GLContext() noexcept
        {
            if (context_handle_) {
                SDL_GL_DeleteContext(context_handle_);
            }
        }

        SDL_GLContext get() { return context_handle_; }

    private:
        friend GLContext GL_CreateContext(SDL_Window* w);
        GLContext(SDL_GLContext _ctx) :
            context_handle_{_ctx}
        {}

        SDL_GLContext context_handle_;
    };

    // https://wiki.libsdl.org/SDL_GL_CreateContext
    inline GLContext GL_CreateContext(SDL_Window* w)
    {
        const SDL_GLContext ctx = SDL_GL_CreateContext(w);

        if (ctx == nullptr) {
            throw std::runtime_error{std::string{"SDL_GL_CreateContext failed: "} + SDL_GetError()};
        }

        return GLContext{ctx};
    }

    // https://wiki.libsdl.org/SDL_GetWindowSizeInPixels
    inline Vec2i GetWindowSizeInPixels(SDL_Window* window)
    {
        // care: `SDL_GetWindowSize` may return a number that's different from
        // the number of pixels in the window on Mac Retina devices
        Vec2i d;
        SDL_GetWindowSizeInPixels(window, &d.x, &d.y);
        return d;
    }
}

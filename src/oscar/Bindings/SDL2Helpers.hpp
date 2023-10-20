#pragma once

#include <SDL.h>
#undef main
#include <glm/vec2.hpp>

#include <oscar/Utils/CStringView.hpp>

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

namespace sdl
{
    // RAII wrapper for SDL_Init and SDL_Quit
    //     https://wiki.libsdl.org/SDL_Quit
    class Context final {
    public:
        explicit Context(Uint32 flags)
        {
            if (SDL_Init(flags) != 0)
            {
                throw std::runtime_error{std::string{"SDL_Init: failed: "} + SDL_GetError()};
            }
        }
        Context(Context const&) = delete;
        Context(Context&&) noexcept = delete;
        Context& operator=(Context const&) = delete;
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

    // RAII wrapper around SDL_Window that calls SDL_DestroyWindow on dtor
    //     https://wiki.libsdl.org/SDL_CreateWindow
    //     https://wiki.libsdl.org/SDL_DestroyWindow
    class Window final {
    public:
        Window(Window const&) = delete;
        Window(Window&& tmp) noexcept :
            m_WindowHandle{std::exchange(tmp.m_WindowHandle, nullptr)}
        {
        }
        Window& operator=(Window const&) = delete;
        Window& operator=(Window&&) noexcept = delete;
        ~Window() noexcept
        {
            if (m_WindowHandle)
            {
                SDL_DestroyWindow(m_WindowHandle);
            }
        }

        SDL_Window* get() const noexcept
        {
            return m_WindowHandle;
        }

        SDL_Window& operator*() const noexcept
        {
            return *m_WindowHandle;
        }

    private:
        friend Window CreateWindoww(osc::CStringView title, int x, int y, int w, int h, Uint32 flags);
        Window(SDL_Window * _ptr) : m_WindowHandle{_ptr}
        {
        }

        SDL_Window* m_WindowHandle;
    };

    // RAII'ed version of SDL_CreateWindow
    //     https://wiki.libsdl.org/SDL_CreateWindow
    //
    // CreateWindoww is a typo because `CreateWindow` is defined in the
    // preprocessor
    inline Window CreateWindoww(osc::CStringView title, int x, int y, int w, int h, Uint32 flags)
    {
        SDL_Window* const win = SDL_CreateWindow(title.c_str(), x, y, w, h, flags);

        if (win == nullptr)
        {
            throw std::runtime_error{std::string{"SDL_CreateWindow failed: "} + SDL_GetError()};
        }

        return Window{win};
    }

    // RAII wrapper around SDL_GLContext that calls SDL_GL_DeleteContext on dtor
    //     https://wiki.libsdl.org/SDL_GL_DeleteContext
    class GLContext final {
    public:
        GLContext(GLContext const&) = delete;
        GLContext(GLContext&& tmp) noexcept :
            m_ContextHandle{std::exchange(tmp.m_ContextHandle, nullptr)}
        {
        }
        GLContext& operator=(GLContext const&) = delete;
        GLContext& operator=(GLContext&&) = delete;
        ~GLContext() noexcept
        {
            if (m_ContextHandle)
            {
                SDL_GL_DeleteContext(m_ContextHandle);
            }
        }

        SDL_GLContext get() noexcept
        {
            return m_ContextHandle;
        }

    private:
        friend GLContext GL_CreateContext(SDL_Window* w);
        GLContext(SDL_GLContext _ctx) : m_ContextHandle{_ctx}
        {
        }

        SDL_GLContext m_ContextHandle;
    };

    // https://wiki.libsdl.org/SDL_GL_CreateContext
    inline GLContext GL_CreateContext(SDL_Window* w)
    {
        SDL_GLContext const ctx = SDL_GL_CreateContext(w);

        if (ctx == nullptr)
        {
            throw std::runtime_error{std::string{"SDL_GL_CreateContext failed: "} + SDL_GetError()};
        }

        return GLContext{ctx};
    }

    // https://wiki.libsdl.org/SDL_GetWindowSize
    inline glm::ivec2 GetWindowSize(SDL_Window* window)
    {
        glm::ivec2 d;
        SDL_GetWindowSize(window, &d.x, &d.y);
        return d;
    }
}

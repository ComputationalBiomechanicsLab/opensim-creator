#pragma once

#include <SDL.h>
#undef main
#include <glm/vec2.hpp>

#include <stdexcept>
#include <string>

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

namespace sdl {

    // RAII wrapper for SDL_Init and SDL_Quit
    //     https://wiki.libsdl.org/SDL_Quit
    class Context final {
    public:
        Context(Uint32 flags)
        {
            if (SDL_Init(flags) != 0)
            {
                throw std::runtime_error{std::string{"SDL_Init: failed: "} + SDL_GetError()};
            }
        }
        Context(Context const&) = delete;
        Context(Context &&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;
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
        Window(Window&& tmp) : m_WindowHandle{tmp.m_WindowHandle}
        {
            tmp.m_WindowHandle = nullptr;
        }
        Window& operator=(Window const&) = delete;
        Window& operator=(Window&& tmp) = delete;
        ~Window() noexcept
        {
            if (m_WindowHandle)
            {
                SDL_DestroyWindow(m_WindowHandle);
            }
        }
        operator SDL_Window*() const noexcept
        {
            return m_WindowHandle;
        }

    private:
        friend Window CreateWindoww(const char* title, int x, int y, int w, int h, Uint32 flags);
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
    inline Window CreateWindoww(const char* title, int x, int y, int w, int h, Uint32 flags)
    {
        SDL_Window* win = SDL_CreateWindow(title, x, y, w, h, flags);

        if (win == nullptr)
        {
            throw std::runtime_error{std::string{"SDL_CreateWindow failed: "} + SDL_GetError()};
        }

        return Window{win};
    }

    // RAII wrapper around an SDL_Renderer that calls SDL_DestroyRenderer on dtor
    //     https://wiki.libsdl.org/SDL_Renderer
    //     https://wiki.libsdl.org/SDL_DestroyRenderer
    class Renderer final {
    public:
        Renderer(Renderer const&) = delete;
        Renderer(Renderer&&) = delete;
        Renderer& operator=(Renderer const&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        ~Renderer() noexcept
        {
            SDL_DestroyRenderer(m_RendererHandle);
        }

        operator SDL_Renderer*() noexcept
        {
            return m_RendererHandle;
        }

    private:
        friend Renderer CreateRenderer(SDL_Window * w, int index, Uint32 flags);
        Renderer(SDL_Renderer * _ptr) : m_RendererHandle{_ptr}
        {
        }

        SDL_Renderer* m_RendererHandle;
    };

    // RAII'ed version of SDL_CreateRenderer
    //     https://wiki.libsdl.org/SDL_CreateRenderer
    inline Renderer CreateRenderer(SDL_Window* w, int index, Uint32 flags)
    {
        SDL_Renderer* r = SDL_CreateRenderer(w, index, flags);

        if (r == nullptr)
        {
            throw std::runtime_error{std::string{"SDL_CreateRenderer: failed: "} + SDL_GetError()};
        }

        return Renderer{r};
    }

    // RAII wrapper around SDL_GLContext that calls SDL_GL_DeleteContext on dtor
    //     https://wiki.libsdl.org/SDL_GL_DeleteContext
    class GLContext final {
    public:
        GLContext(GLContext const&) = delete;
        GLContext(GLContext&& tmp) : m_ContextHandle{tmp.m_ContextHandle}
        {
            tmp.m_ContextHandle = nullptr;
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

        operator SDL_GLContext() noexcept
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
        SDL_GLContext ctx = SDL_GL_CreateContext(w);

        if (ctx == nullptr)
        {
            throw std::runtime_error{std::string{"SDL_GL_CreateContext failed: "} + SDL_GetError()};
        }

        return GLContext{ctx};
    }

    // RAII wrapper for SDL_Surface that calls SDL_FreeSurface on dtor:
    //     https://wiki.libsdl.org/SDL_Surface
    //     https://wiki.libsdl.org/SDL_FreeSurface
    class Surface final {
    public:
        Surface(Surface const&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface const&) = delete;
        Surface& operator=(Surface&&) = delete;
        ~Surface() noexcept
        {
            SDL_FreeSurface(m_SurfaceHandle);
        }

        operator SDL_Surface*() noexcept
        {
            return m_SurfaceHandle;
        }

        SDL_Surface* operator->() const noexcept
        {
            return m_SurfaceHandle;
        }

    private:
        friend Surface CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
        Surface(SDL_Surface* _handle) : m_SurfaceHandle{_handle}
        {
        }

        SDL_Surface* m_SurfaceHandle;
    };

    // RAII'ed version of SDL_CreateRGBSurface:
    //     https://wiki.libsdl.org/SDL_CreateRGBSurface
    inline Surface CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
    {
        SDL_Surface* handle = SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);

        if (handle == nullptr)
        {
            throw std::runtime_error{std::string{"SDL_CreateRGBSurface: "} + SDL_GetError()};
        }

        return Surface{handle};
    }

    // RAII wrapper around SDL_LockSurface/SDL_UnlockSurface:
    //     https://wiki.libsdl.org/SDL_LockSurface
    //     https://wiki.libsdl.org/SDL_UnlockSurface
    class SurfaceLock final {
    public:
        SurfaceLock(SDL_Surface* s) : m_SurfaceHandle{s}
        {
            if (SDL_LockSurface(m_SurfaceHandle) != 0)
            {
                throw std::runtime_error{std::string{"SDL_LockSurface failed: "} + SDL_GetError()};
            }
        }
        SurfaceLock(SurfaceLock const&) = delete;
        SurfaceLock(SurfaceLock&&) = delete;
        SurfaceLock& operator=(SurfaceLock const&) = delete;
        SurfaceLock& operator=(SurfaceLock&&) = delete;
        ~SurfaceLock() noexcept
        {
            SDL_UnlockSurface(m_SurfaceHandle);
        }

    private:
        SDL_Surface* m_SurfaceHandle;
    };

    // RAII'ed version of SDL_LockSurface:
    //     https://wiki.libsdl.org/SDL_LockSurface
    inline SurfaceLock LockSurface(SDL_Surface* s)
    {
        return SurfaceLock{s};
    }

    // RAII wrapper around SDL_Texture that calls SDL_DestroyTexture on dtor:
    //     https://wiki.libsdl.org/SDL_Texture
    //     https://wiki.libsdl.org/SDL_DestroyTexture
    class Texture final {
    public:
        Texture(Texture const&) = delete;
        Texture(Texture&&) = delete;
        Texture& operator=(Texture const&) = delete;
        Texture& operator=(Texture&&) = delete;
        ~Texture() noexcept
        {
            SDL_DestroyTexture(m_TextureHandle);
        }

        operator SDL_Texture*()
        {
            return m_TextureHandle;
        }

    private:
        friend Texture CreateTextureFromSurface(SDL_Renderer* r, SDL_Surface* s);
        Texture(SDL_Texture* _handle) : m_TextureHandle{_handle}
        {
        }

        SDL_Texture* m_TextureHandle;
    };

    // RAII'ed version of SDL_CreateTextureFromSurface:
    //     https://wiki.libsdl.org/SDL_CreateTextureFromSurface
    inline Texture CreateTextureFromSurface(SDL_Renderer* r, SDL_Surface* s)
    {
        SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
        if (t == nullptr)
        {
            throw std::runtime_error{std::string{"SDL_CreateTextureFromSurface failed: "} + SDL_GetError()};
        }
        return Texture{t};
    }

    // https://wiki.libsdl.org/SDL_RenderCopy
    inline void RenderCopy(SDL_Renderer* r, SDL_Texture* t, SDL_Rect* src, SDL_Rect* dest)
    {
        int rv = SDL_RenderCopy(r, t, src, dest);
        if (rv != 0)
        {
            throw std::runtime_error{std::string{"SDL_RenderCopy failed: "} + SDL_GetError()};
        }
    }

    // https://wiki.libsdl.org/SDL_RenderPresent
    inline void RenderPresent(SDL_Renderer* r)
    {
        // this method exists just so that the namespace-based naming is
        // consistent
        SDL_RenderPresent(r);
    }

    struct WindowDimensions {
        int w;
        int h;

        operator glm::vec2() const noexcept
        {
            return {w, h};
        }
    };

    inline bool operator==(WindowDimensions const& a, WindowDimensions const& b) noexcept
    {
        return a.w == b.w && a.h == b.h;
    }

    inline bool operator!=(WindowDimensions const& a, WindowDimensions const& b) noexcept
    {
        return !(a == b);
    }

    // https://wiki.libsdl.org/SDL_GetWindowSize
    inline WindowDimensions GetWindowSize(SDL_Window* window)
    {
        WindowDimensions d;
        SDL_GetWindowSize(window, &d.w, &d.h);
        return d;
    }

    struct MouseState final {
        int x;
        int y;
        Uint32 st;
    };

    // https://wiki.libsdl.org/SDL_GetMouseState
    //
    // mouse state relative to the focus window
    inline MouseState GetMouseState()
    {
        MouseState rv;
        Uint32 st = SDL_GetMouseState(&rv.x, &rv.y);
        rv.st = st;
        return rv;
    }

    using Event = SDL_Event;
    using Rect = SDL_Rect;

    class Timer final {
    public:
        Timer(Timer const&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer const&) = delete;
        Timer& operator=(Timer&&) = delete;
        ~Timer() noexcept
        {
            SDL_RemoveTimer(m_TimerHandle);
        }

    private:
        friend Timer AddTimer(Uint32 interval, SDL_TimerCallback callback, void* param);
        Timer(SDL_TimerID _handle) : m_TimerHandle{_handle}
        {
        }

        SDL_TimerID m_TimerHandle;
    };

    inline Timer AddTimer(Uint32 interval, SDL_TimerCallback callback, void* param)
    {
        SDL_TimerID handle = SDL_AddTimer(interval, callback, param);
        if (handle == 0)
        {
            throw std::runtime_error{std::string{"SDL_AddTimer failed: "} + SDL_GetError()};
        }

        return Timer{handle};
    }
}

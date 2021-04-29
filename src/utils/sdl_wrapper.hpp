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
    // RAII wrapper for SDL_Quit()
    //     https://wiki.libsdl.org/SDL_Quit
    class [[nodiscard]] Context final {
    public:
        Context(Uint32 flags) {
            if (SDL_Init(flags) != 0) {
                throw std::runtime_error{std::string{"SDL_Init: failed: "} + SDL_GetError()};
            }
        }
        Context(Context const&) = delete;
        Context(Context &&) = delete;
        Context& operator=(Context const&) = delete;
        Context& operator=(Context&&) = delete;
        ~Context() noexcept {
            SDL_Quit();
        }
    };

    // https://wiki.libsdl.org/SDL_Init
    inline Context Init(Uint32 flags) {
        return Context{flags};
    }

    // RAII wrapper around SDL_Window that calls SDL_DestroyWindow on dtor
    //     https://wiki.libsdl.org/SDL_CreateWindow
    //     https://wiki.libsdl.org/SDL_DestroyWindow
    class [[nodiscard]] Window final {
        SDL_Window* ptr;

        friend Window CreateWindoww(const char* title, int x, int y, int w, int h, Uint32 flags);
        Window(SDL_Window * _ptr) : ptr{_ptr} {
        }

    public:
        Window(Window const&) = delete;
        Window(Window && tmp) : ptr{tmp.ptr} {
            tmp.ptr = nullptr;
        }
        Window& operator=(Window const&) = delete;
        Window& operator=(Window&& tmp) = delete;
        ~Window() noexcept {
            if (ptr) {
                SDL_DestroyWindow(ptr);
            }
        }
        operator SDL_Window*() const noexcept {
            return ptr;
        }
    };

    // RAII'ed version of SDL_CreateWindow
    //     https://wiki.libsdl.org/SDL_CreateWindow
    //
    // CreateWindoww is a typo because `CreateWindow` is defined in the
    // preprocessor
    inline Window CreateWindoww(const char* title, int x, int y, int w, int h, Uint32 flags) {
        SDL_Window* win = SDL_CreateWindow(title, x, y, w, h, flags);

        if (win == nullptr) {
            throw std::runtime_error{std::string{"SDL_CreateWindow failed: "} + SDL_GetError()};
        }

        return Window{win};
    }

    // RAII wrapper around an SDL_Renderer that calls SDL_DestroyRenderer on dtor
    //     https://wiki.libsdl.org/SDL_Renderer
    //     https://wiki.libsdl.org/SDL_DestroyRenderer
    class [[nodiscard]] Renderer final {
        SDL_Renderer* ptr;

        friend Renderer CreateRenderer(SDL_Window * w, int index, Uint32 flags);
        Renderer(SDL_Renderer * _ptr) : ptr{_ptr} {
        }

    public:
        Renderer(Renderer const&) = delete;
        Renderer(Renderer &&) = delete;
        Renderer& operator=(Renderer const&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        ~Renderer() noexcept {
            SDL_DestroyRenderer(ptr);
        }

        operator SDL_Renderer*() noexcept {
            return ptr;
        }
    };

    // RAII'ed version of SDL_CreateRenderer
    //     https://wiki.libsdl.org/SDL_CreateRenderer
    inline Renderer CreateRenderer(SDL_Window* w, int index, Uint32 flags) {
        SDL_Renderer* r = SDL_CreateRenderer(w, index, flags);

        if (r == nullptr) {
            throw std::runtime_error{std::string{"SDL_CreateRenderer: failed: "} + SDL_GetError()};
        }

        return Renderer{r};
    }

    // RAII wrapper around SDL_GLContext that calls SDL_GL_DeleteContext on dtor
    //     https://wiki.libsdl.org/SDL_GL_DeleteContext
    class GLContext final {
        SDL_GLContext ctx;

        friend GLContext GL_CreateContext(SDL_Window* w);
        GLContext(SDL_GLContext _ctx) : ctx{_ctx} {
        }

    public:
        GLContext(GLContext const&) = delete;
        GLContext(GLContext&& tmp) : ctx{tmp.ctx} {
            tmp.ctx = nullptr;
        }
        GLContext& operator=(GLContext const&) = delete;
        GLContext& operator=(GLContext&&) = delete;
        ~GLContext() noexcept {
            if (ctx) {
                SDL_GL_DeleteContext(ctx);
            }
        }

        operator SDL_GLContext() noexcept {
            return ctx;
        }
    };

    // https://wiki.libsdl.org/SDL_GL_CreateContext
    inline GLContext GL_CreateContext(SDL_Window* w) {
        SDL_GLContext ctx = SDL_GL_CreateContext(w);

        if (ctx == nullptr) {
            throw std::runtime_error{std::string{"SDL_GL_CreateContext failed: "} + SDL_GetError()};
        }

        return GLContext{ctx};
    }

    // RAII wrapper for SDL_Surface that calls SDL_FreeSurface on dtor:
    //     https://wiki.libsdl.org/SDL_Surface
    //     https://wiki.libsdl.org/SDL_FreeSurface
    class Surface final {
        SDL_Surface* handle;

        friend Surface CreateRGBSurface(
            Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
        Surface(SDL_Surface* _handle) : handle{_handle} {
        }

    public:
        Surface(Surface const&) = delete;
        Surface(Surface&&) = delete;
        Surface& operator=(Surface const&) = delete;
        Surface& operator=(Surface&&) = delete;
        ~Surface() noexcept {
            SDL_FreeSurface(handle);
        }

        operator SDL_Surface*() noexcept {
            return handle;
        }
        SDL_Surface* operator->() const noexcept {
            return handle;
        }
    };

    // RAII'ed version of SDL_CreateRGBSurface:
    //     https://wiki.libsdl.org/SDL_CreateRGBSurface
    inline Surface CreateRGBSurface(
            Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
        SDL_Surface* handle = SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);

        if (handle == nullptr) {
            throw std::runtime_error{std::string{"SDL_CreateRGBSurface: "} + SDL_GetError()};
        }

        return Surface{handle};
    }

    // RAII wrapper around SDL_LockSurface/SDL_UnlockSurface:
    //     https://wiki.libsdl.org/SDL_LockSurface
    //     https://wiki.libsdl.org/SDL_UnlockSurface
    class Surface_lock final {
        SDL_Surface* ptr;

    public:
        Surface_lock(SDL_Surface* s) : ptr{s} {
            if (SDL_LockSurface(ptr) != 0) {
                throw std::runtime_error{std::string{"SDL_LockSurface failed: "} + SDL_GetError()};
            }
        }
        Surface_lock(Surface_lock const&) = delete;
        Surface_lock(Surface_lock&&) = delete;
        Surface_lock& operator=(Surface_lock const&) = delete;
        Surface_lock& operator=(Surface_lock&&) = delete;
        ~Surface_lock() noexcept {
            SDL_UnlockSurface(ptr);
        }
    };

    // RAII'ed version of SDL_LockSurface:
    //     https://wiki.libsdl.org/SDL_LockSurface
    inline Surface_lock LockSurface(SDL_Surface* s) {
        return Surface_lock{s};
    }

    // RAII wrapper around SDL_Texture that calls SDL_DestroyTexture on dtor:
    //     https://wiki.libsdl.org/SDL_Texture
    //     https://wiki.libsdl.org/SDL_DestroyTexture
    class Texture final {
        SDL_Texture* handle;

        friend Texture CreateTextureFromSurface(SDL_Renderer* r, SDL_Surface* s);
        Texture(SDL_Texture* _handle) : handle{_handle} {
        }

    public:
        Texture(Texture const&) = delete;
        Texture(Texture&&) = delete;
        Texture& operator=(Texture const&) = delete;
        Texture& operator=(Texture&&) = delete;
        ~Texture() noexcept {
            SDL_DestroyTexture(handle);
        }

        operator SDL_Texture*() {
            return handle;
        }
    };

    // RAII'ed version of SDL_CreateTextureFromSurface:
    //     https://wiki.libsdl.org/SDL_CreateTextureFromSurface
    inline Texture CreateTextureFromSurface(SDL_Renderer* r, SDL_Surface* s) {
        SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
        if (t == nullptr) {
            throw std::runtime_error{std::string{"SDL_CreateTextureFromSurface failed: "} + SDL_GetError()};
        }
        return Texture{t};
    }

    // https://wiki.libsdl.org/SDL_RenderCopy
    inline void RenderCopy(SDL_Renderer* r, SDL_Texture* t, SDL_Rect* src, SDL_Rect* dest) {
        int rv = SDL_RenderCopy(r, t, src, dest);
        if (rv != 0) {
            throw std::runtime_error{std::string{"SDL_RenderCopy failed: "} + SDL_GetError()};
        }
    }

    // https://wiki.libsdl.org/SDL_RenderPresent
    inline void RenderPresent(SDL_Renderer* r) {
        // this method exists just so that the namespace-based naming is
        // consistent
        SDL_RenderPresent(r);
    }

    struct Window_dimensions {
        int w;
        int h;

        operator glm::vec2() const noexcept {
            return {w, h};
        }
    };

    inline bool operator==(Window_dimensions const& a, Window_dimensions const& b) noexcept {
        return a.w == b.w && a.h == b.h;
    }

    inline bool operator!=(Window_dimensions const& a, Window_dimensions const& b) noexcept {
        return !(a == b);
    }

    // https://wiki.libsdl.org/SDL_GetWindowSize
    inline Window_dimensions GetWindowSize(SDL_Window* window) {
        Window_dimensions d;
        SDL_GetWindowSize(window, &d.w, &d.h);
        return d;
    }

    struct Mouse_state final {
        int x;
        int y;
        Uint32 st;
    };

    // https://wiki.libsdl.org/SDL_GetMouseState
    //
    // mouse state relative to the focus window
    inline Mouse_state GetMouseState() {
        Mouse_state rv;
        Uint32 st = SDL_GetMouseState(&rv.x, &rv.y);
        rv.st = st;
        return rv;
    }

    using Event = SDL_Event;
    using Rect = SDL_Rect;

    class Timer final {
        SDL_TimerID handle;

        friend Timer AddTimer(Uint32 interval, SDL_TimerCallback callback, void* param);
        Timer(SDL_TimerID _handle) : handle{_handle} {
        }

    public:
        Timer(Timer const&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer const&) = delete;
        Timer& operator=(Timer&&) = delete;
        ~Timer() noexcept {
            SDL_RemoveTimer(handle);
        }
    };

    inline Timer AddTimer(Uint32 interval, SDL_TimerCallback callback, void* param) {
        SDL_TimerID handle = SDL_AddTimer(interval, callback, param);
        if (handle == 0) {
            throw std::runtime_error{std::string{"SDL_AddTimer failed: "} + SDL_GetError()};
        }

        return Timer{handle};
    }
}

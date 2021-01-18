#include "sdl_wrapper.hpp"

#include <stdexcept>
#include <string>

using std::literals::string_literals::operator""s;

sdl::Window sdl::CreateWindoww(const char* title, int x, int y, int w, int h, Uint32 flags) {
    SDL_Window* win = SDL_CreateWindow(title, x, y, w, h, flags);

    if (win == nullptr) {
        throw std::runtime_error{"SDL_CreateWindow failed: "s + SDL_GetError()};
    }

    return Window{win};
}

sdl::Renderer sdl::CreateRenderer(SDL_Window* w, int index, Uint32 flags) {
    SDL_Renderer* r = SDL_CreateRenderer(w, index, flags);

    if (r == nullptr) {
        throw std::runtime_error{"SDL_CreateRenderer: failed: "s + SDL_GetError()};
    }

    return Renderer{r};
}

sdl::GLContext sdl::GL_CreateContext(SDL_Window* w) {
    SDL_GLContext ctx = SDL_GL_CreateContext(w);

    if (ctx == nullptr) {
        throw std::runtime_error{"SDL_GL_CreateContext failed: "s + SDL_GetError()};
    }

    return GLContext{ctx};
}

sdl::Surface sdl::CreateRGBSurface(
    Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {

    SDL_Surface* handle = SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);

    if (handle == nullptr) {
        throw std::runtime_error{"SDL_CreateRGBSurface: "s + SDL_GetError()};
    }

    return Surface{handle};
}

sdl::Surface_lock::Surface_lock(SDL_Surface* s) : ptr{s} {
    if (SDL_LockSurface(ptr) != 0) {
        throw std::runtime_error{"SDL_LockSurface failed: "s + SDL_GetError()};
    }
}

sdl::Surface_lock sdl::LockSurface(SDL_Surface* s) {
    return Surface_lock{s};
}

sdl::Texture sdl::CreateTextureFromSurface(SDL_Renderer* r, SDL_Surface* s) {
    SDL_Texture* t = SDL_CreateTextureFromSurface(r, s);
    if (t == nullptr) {
        throw std::runtime_error{"SDL_CreateTextureFromSurface failed: "s + SDL_GetError()};
    }
    return Texture{t};
}

void sdl::RenderCopy(SDL_Renderer* r, SDL_Texture* t, SDL_Rect* src, SDL_Rect* dest) {
    int rv = SDL_RenderCopy(r, t, src, dest);
    if (rv != 0) {
        throw std::runtime_error{"SDL_RenderCopy failed: "s + SDL_GetError()};
    }
}

void sdl::RenderPresent(SDL_Renderer* r) {
    // this method exists just so that the namespace-based naming is
    // consistent
    SDL_RenderPresent(r);
}

sdl::Window_dimensions sdl::GetWindowSize(SDL_Window* window) {
    Window_dimensions d;
    SDL_GetWindowSize(window, &d.w, &d.h);
    return d;
}

void sdl::GL_SetSwapInterval(int interval) {
    int rv = SDL_GL_SetSwapInterval(interval);

    if (rv != 0) {
        throw std::runtime_error{"SDL_GL_SetSwapInterval failed: "s + SDL_GetError()};
    }
}

sdl::Timer sdl::AddTimer(Uint32 interval, SDL_TimerCallback callback, void* param) {
    SDL_TimerID handle = SDL_AddTimer(interval, callback, param);
    if (handle == 0) {
        throw std::runtime_error{"SDL_AddTimer failed: "s + SDL_GetError()};
    }

    return Timer{handle};
}

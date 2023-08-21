#pragma once

#include "oscar/Graphics/AntiAliasingLevel.hpp"
#include "oscar/Graphics/Texture2D.hpp"

#include <future>
#include <string>

struct SDL_Window;
namespace osc { struct Color; }

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // graphics context
    //
    // should be initialized exactly once by the application
    class GraphicsContext final {
    public:
        explicit GraphicsContext(SDL_Window&);
        GraphicsContext(GraphicsContext const&) = delete;
        GraphicsContext(GraphicsContext&&) noexcept = delete;
        GraphicsContext& operator=(GraphicsContext const&) = delete;
        GraphicsContext& operator=(GraphicsContext&&) noexcept = delete;
        ~GraphicsContext() noexcept;

        AntiAliasingLevel getMaxAntialiasingLevel() const;

        bool isVsyncEnabled() const;
        void enableVsync();
        void disableVsync();

        bool isInDebugMode() const;
        void enableDebugMode();
        void disableDebugMode();

        void clearProgram();
        void clearScreen(Color const&);

        // HACK: this is needed by ImGui, because it uses OpenGL "in the raw"
        void* updRawGLContextHandle();

        // returns a future that asynchronously yields a complete screenshot of the next frame
        std::future<Texture2D> requestScreenshot();

        // execure the "swap chain" operation, which makes the current backbuffer the frontbuffer,
        void doSwapBuffers(SDL_Window&);

        // human-readable identifier strings: useful for printouts/debugging
        std::string getBackendVendorString() const;
        std::string getBackendRendererString() const;
        std::string getBackendVersionString() const;
        std::string getBackendShadingLanguageVersionString() const;

        class Impl;
    private:
        // no data - it uses globals (you can only have one of these, globally)
    };
}

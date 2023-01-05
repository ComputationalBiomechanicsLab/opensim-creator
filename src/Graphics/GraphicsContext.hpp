#pragma once

#include "src/Graphics/Image.hpp"

#include <glm/vec4.hpp>

#include <cstdint>
#include <future>
#include <string>

struct SDL_Window;

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

        int32_t getMaxMSXAASamples() const;

        bool isVsyncEnabled() const;
        void enableVsync();
        void disableVsync();

        bool isInDebugMode() const;
        void enableDebugMode();
        void disableDebugMode();

        void clearProgram();
        void clearScreen(glm::vec4 const&);

        // HACK: this is needed by ImGui, because it uses OpenGL "in the raw"
        void* updRawGLContextHandle();

        // returns a future that asynchronously yields a complete screenshot of the next frame
        std::future<Image> requestScreenshot();

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
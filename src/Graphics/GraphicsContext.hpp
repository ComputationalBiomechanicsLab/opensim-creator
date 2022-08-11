#pragma once

#include <glm/vec4.hpp>

#include <string>

struct SDL_Window;

// note: implementation is in `Renderer.cpp`
namespace osc
{
    // graphics context
    //
    // should be initialized exactly once by the application
    class GraphicsContext final {
    public:
        GraphicsContext(SDL_Window*);
        GraphicsContext(GraphicsContext const&) = delete;
        GraphicsContext(GraphicsContext&&) noexcept = delete;
        GraphicsContext& operator=(GraphicsContext const&) = delete;
        GraphicsContext& operator=(GraphicsContext&&) noexcept = delete;
        ~GraphicsContext() noexcept;

        int getMaxMSXAASamples() const;

        bool isVsyncEnabled() const;
        void enableVsync();
        void disableVsync();

        bool isInDebugMode() const;
        void enableDebugMode();
        void disableDebugMode();

        void clearProgram();
        void clearScreen(glm::vec4 const&);

        void* updRawGLContextHandle();  // needed by ImGui

        // human-readable identifier strings: useful for printouts/debugging
        std::string getBackendVendorString() const;
        std::string getBackendRendererString() const;
        std::string getBackendVersionString() const;
        std::string getBackendShadingLanguageVersionString() const;

        class Impl;
    private:
        // no data - it uses globals (you can only have one of these)
    };
}
#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Graphics/Texture2D.h>

#include <future>
#include <string>

struct SDL_Window;
namespace osc { struct Color; }

namespace osc
{
    // graphics context
    //
    // should be initialized exactly once by the application
    class GraphicsContext final {
    public:
        explicit GraphicsContext(SDL_Window&);
        GraphicsContext(const GraphicsContext&) = delete;
        GraphicsContext(GraphicsContext&&) noexcept = delete;
        GraphicsContext& operator=(const GraphicsContext&) = delete;
        GraphicsContext& operator=(GraphicsContext&&) noexcept = delete;
        ~GraphicsContext() noexcept;

        AntiAliasingLevel max_antialiasing_level() const;

        bool is_vsync_enabled() const;
        void enable_vsync();
        void disable_vsync();

        bool is_in_debug_mode() const;
        void enable_debug_mode();
        void disable_debug_mode();

        void clear_screen(const Color&);

        // HACK: this is needed by ImGui's SDL backend, because it uses OpenGL "in the raw"
        void* upd_raw_opengl_context_handle_HACK();

        // returns a future that asynchronously yields a complete screenshot of the next complete frame
        std::future<Texture2D> request_screenshot();

        // execute the "swap chain" operation, which makes the current backbuffer the frontbuffer and
        // the frontbuffer the backbuffer
        void swap_buffers(SDL_Window&);

        // human-readable identifier strings: useful for printouts/debugging
        std::string backend_vendor_string() const;
        std::string backend_renderer_string() const;
        std::string backend_version_string() const;
        std::string backend_shading_language_version_string() const;

        class Impl;
    private:
        // no data - it uses globals (you can only have one of these, globally)
    };
}

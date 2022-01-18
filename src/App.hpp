#pragma once

#include "src/3D/ShaderCache.hpp"
#include "src/Assertions.hpp"
#include "src/MeshCache.hpp"
#include "src/RecentFile.hpp"
#include "src/Screen.hpp"

#include <SDL_events.h>
#include <glm/vec2.hpp>

#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace osc
{
    struct Config;
    class ShaderCache;
}

namespace osc
{
    class App final {
    public:

        static App& cur() noexcept
        {
            OSC_ASSERT(g_Current && "App is not initialized: have you constructed a (singleton) instance of App?");
            return *g_Current;
        }
        static Config const& config() noexcept;
        static ShaderCache& shaders() noexcept;
        static MeshCache& meshes() noexcept;
        static std::filesystem::path resource(std::string_view s);

        template<typename TShader>
        static TShader& shader()
        {
            return shaders().getShader<TShader>();
        }

        // init app by loading config from default location
        App();
        App(App const&) = delete;
        App(App&&) noexcept;
        ~App() noexcept;

        App& operator=(App const&) = delete;
        App& operator=(App&&) noexcept;

        // start showing the supplied screen
        void show(std::unique_ptr<Screen>);

        // construct `TScreen` with `Args` and start showing it
        template<typename TScreen, typename... Args>
        void show(Args&&... args)
        {
            show(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // request the app transitions to a new sreen
        //
        // this is a *request* that `App` will fulfill at a later time. App will
        // first unmount the current screen, fully destroy the current
        // screen, then mount the new screen and make the new screen
        // the current screen
        void requestTransition(std::unique_ptr<Screen>);

        // construct `TScreen` with `Args` then request the app transitions to it
        template<typename TScreen, typename... Args>
        void requestTransition(Args&&... args)
        {
            requestTransition(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // request the app quits as soon as it can (usually after it's finished with a
        // screen method)
        void requestQuit() noexcept;

        // returns current window dimensions (integer)
        glm::ivec2 idims() const noexcept;

        // returns current window dimensions (float)
        glm::vec2 dims() const noexcept;

        // returns current window aspect ratio
        float aspectRatio() const noexcept;

        // sets whether the application should show/hide the cursor
        void showCursor(bool) noexcept;

        // makes the application window fullscreen
        void makeFullscreen();

        // makes the application window windowed (as opposed to fullscreen)
        void makeWindowed();

        // returns number of MSXAA samples multisampled rendererers should use
        int getSamples() const noexcept;

        // sets the number of MSXAA samples multisampled renderered should use
        //
        // throws if arg > max_samples()
        void setSamples(int);

        // returns the maximum number of MSXAA samples the backend supports
        int maxSamples() const noexcept;

        // returns true if the application is rendering in debug mode
        //
        // screen/tab/widget implementations should use this to decide whether
        // to draw extra debug elements
        bool isInDebugMode() const noexcept;
        void enableDebugMode();
        void disableDebugMode();

        bool isVsyncEnabled() const noexcept;
        void enableVsync();
        void disableVsync();

        Config const& getConfig() const noexcept;

        // get full path to runtime resource in `resources/` dir
        std::filesystem::path getResource(std::string_view) const noexcept;

        // returns the contents of a resource in a string
        std::string slurpResource(std::string_view) const;

        // returns all files that were recently opened by the user in the app
        //
        // the list is persisted between app boots
        std::vector<RecentFile> getRecentFiles() const;

        // add a file to the recently opened files list
        //
        // this addition is persisted between app boots
        void addRecentFile(std::filesystem::path const&);

        // returns true if the main app window is focused
        bool isWindowFocused() const noexcept;

        struct MouseState final {
            glm::ivec2 pos;
            bool LeftDown;
            bool RightDown;
            bool MiddleDown;
            bool X1Down;
            bool X2Down;
        };

        // get current mouse state
        //
        // note: this tries to be as precise as possible by fetching from the
        //       OS if possible, so it can be expensive
        MouseState getMouseState() const noexcept;

        // returns the number of "ticks" that the application has counted
        uint64_t getTicks() const noexcept;

        // returns the number of "ticks" the application accumulates per second
        uint64_t getTickFrequency() const noexcept;

        // returns true if the user is pressing the SHIFT key
        bool isShiftPressed() const noexcept;

        // returns true if the user is pressing the CTRL key
        bool isCtrlPressed() const noexcept;

        // returns true if the user is pressing the ALT key
        bool isAltPressed() const noexcept;

        // move the mouse to a location within the window
        void warpMouseInWindow(glm::vec2) const noexcept;

        // returns the application-wide shader cache
        ShaderCache& getShaderCache() noexcept;

        // returns the application-wide mesh cache
        MeshCache& getMeshCache() noexcept;

        // makes main application event loop wait, rather than poll, for events
        //
        // this transforms the application from being a game-like loop (continuously
        // re-render) into something closer to an application. Downstream screens will
        // have to call `requestRedraw` to force the application to redraw when there isn't
        // and OS event
        void makeMainEventLoopWaiting();

        // makes the main application event loop poll, rather than waiting, for events
        //
        // this makes the application game-like, and doesn't require that downstream screens
        // call `requestRedraw` whenever they want to force a redraw
        void makeMainEventLoopPolling();

        // threadsafe: pumps a redraw event into the application's event loop
        void requestRedraw();

        struct Impl;
    private:
        friend void ImGuiInit();
        friend void ImGuiNewFrame();

        // set when App is constructed for the first time
        static App* g_Current;
        std::unique_ptr<Impl> m_Impl;
    };

    // ImGui support
    //
    // these methods are specialized for OSC (config, fonts, themeing, etc.)
    //
    // these methods should be called by each `Screen` implementation. The reason they aren't
    // automatically integrated into App/Screen is because some screens might want very tight
    // control over ImGui (e.g. recycling contexts, aggro-resetting contexts)

    void ImGuiInit();  // init ImGui context (/w osc settings)
    void ImGuiShutdown();  // shutdown ImGui context
    bool ImGuiOnEvent(SDL_Event const&);  // returns true if ImGui has handled the event
    void ImGuiNewFrame();  // should be called at the start of `draw()`
    void ImGuiRender();  // should be called at the end of `draw()`
}

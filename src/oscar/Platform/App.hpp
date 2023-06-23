#pragma once

#include "oscar/Graphics/AnnotatedImage.hpp"
#include "oscar/Graphics/Image.hpp"
#include "oscar/Platform/AppClock.hpp"
#include "oscar/Platform/Log.hpp"
#include "oscar/Platform/MouseState.hpp"
#include "oscar/Platform/RecentFile.hpp"
#include "oscar/Utils/Assertions.hpp"

#include <SDL_events.h>
#include <glm/vec2.hpp>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <future>
#include <functional>
#include <memory>
#include <mutex>
#include <ratio>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

namespace osc { struct Color; }
namespace osc { class Config; }
namespace osc { class Screen; }

namespace osc
{
    // top-level application class
    //
    // the top-level osc process holds one copy of this class, which maintains all global
    // systems (windowing, event pumping, timers, graphics, logging, etc.)
    class App {
    public:
        template<typename T, typename...Args>
        static std::shared_ptr<T> singleton(Args&&... args)
        {
            auto const ctor = [argTuple = std::make_tuple(std::forward<Args>(args)...)]() mutable
            {
                return std::apply([](auto&&... innerArgs) -> std::shared_ptr<void>
                {
                     auto const customDeleter = [](T* t) { delete t; };
                     return std::shared_ptr<void>{new T{std::forward<Args>(innerArgs)...}, customDeleter};
                }, std::move(argTuple));
            };

            return std::static_pointer_cast<T>(App::upd().updSingleton(typeid(T), ctor));
        }

        // returns the currently-active application global
        static App& upd()
        {
            OSC_ASSERT(g_Current && "App is not initialized: have you constructed a (singleton) instance of App?");
            return *g_Current;
        }

        static App const& get()
        {
            OSC_ASSERT(g_Current && "App is not initialized: have you constructed a (singleton) instance of App?");
            return *g_Current;
        }

        static Config const& config()
        {
            return get().getConfig();
        }

        // returns a full filesystem path to a (runtime- and configuration-dependent) application resource
        static std::filesystem::path resource(std::string_view);

        // returns the contents of a runtime resource in the `resources/` dir as a string
        static std::string slurp(std::string_view);

        // returns the contents of a runtime resource in the `resources/` dir as a binary blob
        static std::vector<uint8_t> slurpBinary(std::string_view);

        // constructs an app by initializing it from a config at the default app config location
        //
        // this also sets the `cur` application global
        App();
        App(App const&) = delete;
        App(App&&) noexcept;
        App& operator=(App const&) = delete;
        App& operator=(App&&) noexcept;
        ~App() noexcept;

        // start showing the supplied screen, only returns once a screen requests to quit or an exception is thrown
        void show(std::unique_ptr<Screen>);

        // construct `TScreen` with `Args` and start showing it
        template<typename TScreen, typename... Args>
        void show(Args&&... args)
        {
            static_assert(std::is_base_of_v<Screen, TScreen>);
            show(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // Request the app transitions to a new sreen
        //
        // this is merely a *request* that the `App` will fulfill at a later
        // time (usually, after it's done handling some part of the top-level application
        // loop).
        //
        // When the App decides it's ready to transition to the new screen, it will:
        //
        // - unmount the current screen
        // - destroy the current screen
        // - mount the new screen
        // - make the new screen the current screen
        void requestTransition(std::unique_ptr<Screen>);

        // construct `TScreen` with `Args` then request the app transitions to it
        template<typename TScreen, typename... Args>
        void requestTransition(Args&&... args)
        {
            requestTransition(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // returns `true` if a screen transition is pending and will probably happen at
        // the next point in the application loop (e.g. after event polling, drawing)
        bool isTransitionRequested() const;

        // request that the app quits
        //
        // this is merely a *request* tha the `App` will fulfill at a later time (usually,
        // after it's done handling some part of the top-level application loop)
        void requestQuit();

        // returns main window's dimensions (integer)
        glm::ivec2 idims() const;

        // returns main window's dimensions (float)
        glm::vec2 dims() const;

        // returns main window's aspect ratio
        float aspectRatio() const;

        // sets whether the user's mouse cursor should be shown/hidden
        void setShowCursor(bool);

        // returns true if the main window is focused
        bool isWindowFocused() const;

        // makes the main window fullscreen
        void makeFullscreen();

        // makes the main window fullscreen, but still composited with the desktop (so-called 'windowed maximized' in games)
        void makeWindowedFullscreen();

        // makes the main window windowed (as opposed to fullscreen)
        void makeWindowed();

        // returns the recommended number of MSXAA samples that rendererers should use (based on config etc.)
        int32_t getMSXAASamplesRecommended() const;

        // sets the number of MSXAA samples multisampled renderered should use
        //
        // throws if arg > max_samples()
        void setMSXAASamplesRecommended(int32_t);

        // returns the maximum number of MSXAA samples the backend supports
        int32_t getMSXAASamplesMax() const;

        // returns true if the application is rendering in debug mode
        //
        // other parts of the application can use this to decide whether to render
        // extra debug elements, etc.
        bool isInDebugMode() const;
        void enableDebugMode();
        void disableDebugMode();

        // returns true if VSYNC has been enabled in the graphics layer
        bool isVsyncEnabled() const;
        void setVsync(bool);
        void enableVsync();
        void disableVsync();

        // add an annotation to the current frame
        //
        // the annotation is added to the data returned by `App::requestAnnotatedScreenshot`
        void addFrameAnnotation(std::string_view label, Rect screenRect);

        // returns a future that asynchronously yields a complete screenshot of the next frame
        std::future<Image> requestScreenshot();

        // returns a future that asynchronously yields a complete annotated screenshot of the next frame
        //
        // client code can submit annotations with `App::addFrameAnnotation`
        std::future<AnnotatedImage> requestAnnotatedScreenshot();

        // returns human-readable strings representing (parts of) the graphics backend (e.g. OpenGL)
        std::string getGraphicsBackendVendorString() const;
        std::string getGraphicsBackendRendererString() const;
        std::string getGraphicsBackendVersionString() const;
        std::string getGraphicsBackendShadingLanguageVersionString() const;

        // returns the number of times the application has drawn a frame to the screen
        uint64_t getFrameCount() const;

        // returns the number of "ticks" recorded on the application's high-resolution
        // monotonically-increasing clock
        //
        // care: this always fetches from the underlying platform API, so should only really
        //       be used infrequently: animations etc. should use the frame-based clocks|
        uint64_t getTicks() const;

        // returns the number of "ticks" that pass in the application's high-resolution
        // clock per second.
        //
        // usage e.g.: dt = (getTicks()-previousTicks)/getTickFrequency()
        //
        // care: this always fetches from the underlying platform API, so should only really
        //       be used infrequently: animations etc. should use the frame-based clocks|
        uint64_t getTickFrequency() const;

        AppClock::time_point getCurrentTime() const;  // care: always fetches the time *right now*
        AppClock::time_point getAppStartupTime() const;
        AppClock::time_point getFrameStartTime() const;
        AppClock::duration getDeltaSinceAppStartup() const;
        AppClock::duration getDeltaSinceLastFrame() const;

        // makes main application event loop wait, rather than poll, for events
        //
        // By default, `App` is a *polling* event loop that renders as often as possible. This
        // method makes the main application a *waiting* event loop that only moves forward when
        // an event occurs.
        //
        // Rendering this way is *much* more power efficient (especially handy on TDP-limited devices
        // like laptops), but downstream screens *must* ensure the application keeps moving forward by
        // calling methods like `requestRedraw` or by pumping other events into the loop.
        bool isMainLoopWaiting() const;
        void setMainLoopWaiting(bool);
        void makeMainEventLoopWaiting();
        void makeMainEventLoopPolling();
        void requestRedraw();  // threadsafe: used to make a waiting loop redraw

        // fill all pixels in the screen with the given color
        void clearScreen(Color const&);

        // get the user's current mouse state
        //
        // note: this method tries to be as precise as possible by fetching from the
        //       OS, so it can be expensive. Use something like an IoPoller or ImGui
        //       to record this information once-per-frame, if possible.
        MouseState getMouseState() const;

        // move the mouse to a location within the window
        void warpMouseInWindow(glm::vec2) const;

        // returns true if the user is pressing the SHIFT key
        //
        // note: this might fetch the information from the OS, so it's better to store
        //       it once-per-frame in something like an IoPoller
        bool isShiftPressed() const;

        // returns true if the user is pressing the CTRL key
        //
        // note: this might fetch the information from the OS, so it's better to store
        //       it once-per-frame in something like an IoPoller
        bool isCtrlPressed() const;

        // returns true if the user is pressing the ALT key
        //
        // note: this might fetch the information from the OS, so it's better to store
        //       it once-per-frame in something like an IoPoller
        bool isAltPressed() const;

        // sets the main window's subtitle (e.g. document name)
        void setMainWindowSubTitle(std::string_view);

        // unsets the main window's subtitle
        void unsetMainWindowSubTitle();

        // returns the current application configuration
        Config const& getConfig() const;
        Config& updConfig();

        // returns a full filesystem path to runtime resource in `resources/` dir
        std::filesystem::path getResource(std::string_view) const;

        // returns the contents of a runtime resource in the `resources/` dir as a string
        std::string slurpResource(std::string_view) const;

        // returns the contents of a runtime resource in the `resources/` dir as an unsigned byte sequence
        std::vector<uint8_t> slurpBinaryResource(std::string_view) const;

        // returns all files that were recently opened by the user in the app
        //
        // the list is persisted between app boots
        std::vector<RecentFile> getRecentFiles() const;

        // add a file to the recently opened files list
        //
        // this addition is persisted between app boots
        void addRecentFile(std::filesystem::path const&);

    private:
        // try and retrieve a virtual singleton that has the same lifetime as the app
        std::shared_ptr<void> updSingleton(std::type_info const&, std::function<std::shared_ptr<void>()> const&);

        friend void ImGuiInit();
        friend void ImGuiNewFrame();
        friend void ImGuiRender();

        // set when App is constructed for the first time
        static App* g_Current;
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };

    // ImGui support
    //
    // these methods are specialized for OSC (config, fonts, themeing, etc.)
    //
    // these methods should be called by each `Screen` implementation. The reason they aren't
    // automatically integrated into App/Screen is because some screens might want very tight
    // control over ImGui (e.g. recycling contexts, aggro-resetting contexts)

    // init ImGui context (/w osc settings)
    void ImGuiInit();

    // shutdown ImGui context
    void ImGuiShutdown();

    // returns true if ImGui has handled the event
    bool ImGuiOnEvent(SDL_Event const&);

    // should be called at the start of `draw()`
    void ImGuiNewFrame();

    // should be called at the end of `draw()`
    void ImGuiRender();
}

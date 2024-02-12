#pragma once

#include <oscar/Graphics/AntiAliasingLevel.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Platform/AppClock.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourceStream.h>
#include <oscar/Platform/Screenshot.h>

#include <SDL_events.h>

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

struct SDL_Window;
namespace osc { struct Color; }
namespace osc { class AppConfig; }
namespace osc { class AppMetadata; }
namespace osc { class IScreen; }
namespace osc::ui::context { void Init(); }

namespace osc
{
    // top-level application class
    //
    // the top-level osc process holds one copy of this class, which maintains all global
    // systems (windowing, event pumping, timers, graphics, logging, etc.)
    class App {
    public:
        template<std::destructible T, typename... Args>
        static std::shared_ptr<T> singleton(Args&&... args)
            requires std::constructible_from<T, Args&&...>
        {
            auto const ctor = [argTuple = std::make_tuple(std::forward<Args>(args)...)]() mutable -> std::shared_ptr<void>
            {
                return std::apply([](auto&&... innerArgs) -> std::shared_ptr<void>
                {
                     return std::make_shared<T>(std::forward<Args>(innerArgs)...);
                }, std::move(argTuple));
            };

            return std::static_pointer_cast<T>(App::upd().updSingleton(typeid(T), ctor));
        }

        // returns the currently-active application global
        static App& upd();
        static App const& get();

        static AppConfig const& config();

        // returns a full filesystem path to a (runtime- and configuration-dependent) application resource
        static std::filesystem::path resourceFilepath(ResourcePath const&);

        // returns the contents of a runtime resource in the `resources/` dir as a string
        static std::string slurp(ResourcePath const&);

        // returns an opened stream to the given application resource
        static ResourceStream load_resource(ResourcePath const&);

        // returns the top- (application-)level resource loader
        static ResourceLoader& resource_loader();

        // constructs an `App` from a default-constructed `AppMetadata`
        App();

        // constructs an app by initializing it from a config at the default app config location
        //
        // this also sets the `cur` application global
        explicit App(AppMetadata const&);
        App(App const&) = delete;
        App(App&&) noexcept;
        App& operator=(App const&) = delete;
        App& operator=(App&&) noexcept;
        ~App() noexcept;

        // returns the application's metdata (name, organization, repo URL, version, etc.)
        AppMetadata const& getMetadata() const;

        // returns the filesystem path to the current application executable
        std::filesystem::path const& getExecutableDirPath() const;

        // returns the filesystem path to a (usually, writable) user-specific directory for the
        // application
        std::filesystem::path const& getUserDataDirPath() const;

        // start showing the supplied screen, only returns once a screen requests to quit or an exception is thrown
        void show(std::unique_ptr<IScreen>);

        // construct `TScreen` with `Args` and start showing it
        template<std::derived_from<IScreen> TScreen, typename... Args>
        void show(Args&&... args)
            requires std::constructible_from<TScreen, Args&&...>
        {
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
        void requestTransition(std::unique_ptr<IScreen>);

        // construct `TScreen` with `Args` then request the app transitions to it
        template<std::derived_from<IScreen> TScreen, typename... Args>
        void requestTransition(Args&&... args)
            requires std::constructible_from<TScreen, Args&&...>
        {
            requestTransition(std::make_unique<TScreen>(std::forward<Args>(args)...));
        }

        // request that the app quits
        //
        // this is merely a *request* tha the `App` will fulfill at a later time (usually,
        // after it's done handling some part of the top-level application loop)
        void requestQuit();

        // returns main window's dimensions (float)
        Vec2 dims() const;

        // sets whether the user's mouse cursor should be shown/hidden
        void setShowCursor(bool);

        // makes the main window fullscreen
        void makeFullscreen();

        // makes the main window fullscreen, but still composited with the desktop (so-called 'windowed maximized' in games)
        void makeWindowedFullscreen();

        // makes the main window windowed (as opposed to fullscreen)
        void makeWindowed();

        // returns the recommended number of MSXAA antiAliasingLevel that rendererers should use (based on config etc.)
        AntiAliasingLevel getCurrentAntiAliasingLevel() const;

        // sets the number of MSXAA antiAliasingLevel multisampled renderered should use
        //
        // throws if arg > max_samples()
        void setCurrentAntiAliasingLevel(AntiAliasingLevel);

        // returns the maximum number of MSXAA antiAliasingLevel the backend supports
        AntiAliasingLevel getMaxAntiAliasingLevel() const;

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
        // the annotation is added to the data returned by `App::requestScreenshot`
        void addFrameAnnotation(std::string_view label, Rect screenRect);

        // returns a future that asynchronously yields a complete annotated screenshot of the next frame
        //
        // client code can submit annotations with `App::addFrameAnnotation`
        std::future<Screenshot> requestScreenshot();

        // returns human-readable strings representing (parts of) the graphics backend (e.g. OpenGL)
        std::string getGraphicsBackendVendorString() const;
        std::string getGraphicsBackendRendererString() const;
        std::string getGraphicsBackendVersionString() const;
        std::string getGraphicsBackendShadingLanguageVersionString() const;

        // returns the number of times the application has drawn a frame to the screen
        uint64_t getFrameCount() const;

        // returns the time at which the app started up (arbitrary timepoint, don't assume 0)
        AppClock::time_point getAppStartupTime() const;

        // returns the time delta between when the app started up and the current frame
        AppClock::duration getFrameDeltaSinceAppStartup() const;

        // returns the time at which the current frame started
        AppClock::time_point getFrameStartTime() const;

        // returns the time delta between when the current frame started and when the previous
        // frame started
        AppClock::duration getFrameDeltaSinceLastFrame() const;

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

        // sets the main window's subtitle (e.g. document name)
        void setMainWindowSubTitle(std::string_view);

        // unsets the main window's subtitle
        void unsetMainWindowSubTitle();

        // returns the current application configuration
        AppConfig const& getConfig() const;
        AppConfig& updConfig();

        // returns the top- (application-)level resource loader
        ResourceLoader& updResourceLoader();

        // returns the contents of a runtime resource in the `resources/` dir as a string
        std::string slurpResource(ResourcePath const&);

        // returns an opened stream to the given resource
        ResourceStream loadResource(ResourcePath const&);

    private:
        // returns a full filesystem path to runtime resource in `resources/` dir
        std::filesystem::path getResourceFilepath(ResourcePath const&) const;

        // try and retrieve a virtual singleton that has the same lifetime as the app
        std::shared_ptr<void> updSingleton(std::type_info const&, std::function<std::shared_ptr<void>()> const&);

        // HACK: the 2D ui currently needs access to these
        SDL_Window* updUndleryingWindow();
        void* updUnderlyingOpenGLContext();
        friend void ui::context::Init();

        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

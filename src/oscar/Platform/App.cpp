#include "App.hpp"

#include <oscar/Graphics/GraphicsContext.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Platform/AppClock.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Platform/AppMetadata.hpp>
#include <oscar/Platform/IResourceLoader.hpp>
#include <oscar/Platform/IScreen.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/ResourceLoader.hpp>
#include <oscar/Platform/ResourcePath.hpp>
#include <oscar/Platform/ResourceStream.hpp>
#include <oscar/Platform/Screenshot.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Platform/Detail/SDL2Helpers.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/FilesystemHelpers.hpp>
#include <oscar/Utils/Perf.hpp>
#include <oscar/Utils/ScopeGuard.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <SDL.h>
#include <SDL_error.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>
#include <SDL_video.h>

#include <cmath>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace sdl = osc::sdl;
using namespace osc;

namespace
{
    App* g_ApplicationGlobal = nullptr;

    void Sdl_GL_SetAttributeOrThrow(
        SDL_GLattr attr,
        CStringView attrReadableName,
        int value,
        CStringView valueReadableName)
    {
        if (SDL_GL_SetAttribute(attr, value))
        {
            std::stringstream msg;
            msg << "SDL_GL_SetAttribute failed when setting " << attrReadableName << " = " << valueReadableName << ": " << SDL_GetError();
            throw std::runtime_error{std::move(msg).str()};
        }
    }

    // install backtrace dumper
    //
    // useful if the application fails in prod: can provide some basic backtrace
    // info that users can paste into an issue or something, which is *a lot* more
    // information than "yeah, it's broke"
    bool EnsureBacktraceHandlerEnabled(std::filesystem::path const& crashDumpDir)
    {
        log_info("enabling backtrace handler");
        InstallBacktraceHandler(crashDumpDir);
        return true;
    }

    bool ConfigureApplicationLog(AppConfig const& config)
    {
        if (auto logger = defaultLogger())
        {
            logger->set_level(config.getRequestedLogLevel());
        }
        return true;
    }

    // initialize the main application window
    sdl::Window CreateMainAppWindow(CStringView applicationName)
    {
        log_info("initializing main application window");

        Sdl_GL_SetAttributeOrThrow(SDL_GL_CONTEXT_PROFILE_MASK, "SDL_GL_CONTEXT_PROFILE_MASK", SDL_GL_CONTEXT_PROFILE_CORE, "SDL_GL_CONTEXT_PROFILE_CORE");
        Sdl_GL_SetAttributeOrThrow(SDL_GL_CONTEXT_MAJOR_VERSION, "SDL_GL_CONTEXT_MAJOR_VERSION", 3, "3");
        Sdl_GL_SetAttributeOrThrow(SDL_GL_CONTEXT_MINOR_VERSION, "SDL_GL_CONTEXT_MINOR_VERSION", 3, "3");
        Sdl_GL_SetAttributeOrThrow(SDL_GL_CONTEXT_FLAGS, "SDL_GL_CONTEXT_FLAGS", SDL_GL_CONTEXT_DEBUG_FLAG, "SDL_GL_CONTEXT_DEBUG_FLAG");
        Sdl_GL_SetAttributeOrThrow(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, "SDL_GL_FRAMEBUFFER_SRGB_CAPABLE", 1, "1");

        // careful about setting resolution, position, etc. - some people have *very* shitty
        // screens on their laptop (e.g. ultrawide, sub-HD, minus space for the start bar, can
        // be <700 px high)
        constexpr int x = SDL_WINDOWPOS_CENTERED;
        constexpr int y = SDL_WINDOWPOS_CENTERED;
        constexpr int width = 800;
        constexpr int height = 600;
        constexpr Uint32 flags =
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;

        return sdl::CreateWindoww(applicationName.c_str(), x, y, width, height, flags);
    }

    AppClock::duration ConvertPerfTicksToFClockDuration(Uint64 ticks, Uint64 frequency)
    {
        auto const dticks = static_cast<double>(ticks);
        auto const dfrequency = static_cast<double>(frequency);
        auto const duration = static_cast<AppClock::rep>(dticks/dfrequency);

        return AppClock::duration{duration};
    }

    AppClock::time_point ConvertPerfCounterToFClock(Uint64 ticks, Uint64 frequency)
    {
        return AppClock::time_point{ConvertPerfTicksToFClockDuration(ticks, frequency)};
    }

    std::filesystem::path GetCurrentExeDirAndLogIt()
    {
        auto rv = CurrentExeDir();
        log_info("executable directory: %s", rv.string().c_str());
        return rv;
    }

    // computes the user's data directory and also logs it to the console for user-facing feedback
    std::filesystem::path GetUserDataDirAndLogIt(
        CStringView organizationName,
        CStringView applicationName)
    {
        auto rv = GetUserDataDir(organizationName, applicationName);
        log_info("user data directory: %s", rv.string().c_str());
        return rv;
    }

    class AppResourceLoader final : public IResourceLoader {
    public:
        AppResourceLoader(std::shared_ptr<AppConfig> config_) :
            m_Config{std::move(config_)}
        {}

        ResourceStream implOpen(ResourcePath const& p)
        {
            if (log_level() <= LogLevel::debug) {
                log_debug("opening %s", p.string().c_str());
            }

            auto fsPath = std::filesystem::weakly_canonical(m_Config->getResourceDir() / p.string());
            return ResourceStream{fsPath};
        }
    private:
        std::shared_ptr<AppConfig> m_Config;
    };
}

namespace
{
    // an "active" request for an annotated screenshot
    //
    // has a data depencency on the backend first providing a "raw" image, which is then
    // tagged with annotations
    struct AnnotatedScreenshotRequest final {

        AnnotatedScreenshotRequest(
            uint64_t frameRequested_,
            std::future<Texture2D> underlyingFuture_) :

            frameRequested{frameRequested_},
            underlyingScreenshotFuture{std::move(underlyingFuture_)}
        {
        }

        // the frame on which the screenshot was requested
        uint64_t frameRequested;

        // underlying (to-be-waited-on) future for the screenshot
        std::future<Texture2D> underlyingScreenshotFuture;

        // our promise to the caller, who is waiting for an annotated image
        std::promise<Screenshot> resultPromise;

        // annotations made during the requested frame (if any)
        std::vector<ScreenshotAnnotation> annotations;
    };

    // wrapper class for storing std::type_info as a hashable type
    class TypeInfoReference final {
    public:
        explicit TypeInfoReference(std::type_info const& typeInfo) :
            m_TypeInfo{&typeInfo}
        {
        }

        std::type_info const& get() const
        {
            return *m_TypeInfo;
        }

        friend bool operator==(TypeInfoReference const& lhs, TypeInfoReference const& rhs)
        {
            return lhs.get() == rhs.get();
        }
    private:
        std::type_info const* m_TypeInfo;
    };
}

template<>
struct std::hash<TypeInfoReference> final {
    size_t operator()(TypeInfoReference const& ref) const
    {
        return ref.get().hash_code();
    }
};

// main application state
//
// this is what "booting the application" actually initializes
class osc::App::Impl final {
public:
    explicit Impl(AppMetadata const& metadata_) :  // NOLINT(modernize-pass-by-value)
        m_Metadata{metadata_}
    {
    }

    AppMetadata const& getMetadata() const
    {
        return m_Metadata;
    }

    std::filesystem::path const& getExecutableDirPath() const
    {
        return m_ExecutableDirPath;
    }

    std::filesystem::path const& getUserDataDirPath() const
    {
        return m_UserDataDirPath;
    }

    void show(std::unique_ptr<IScreen> s)
    {
        log_info("showing screen %s", s->getName().c_str());

        if (m_CurrentScreen)
        {
            throw std::runtime_error{"tried to call App::show when a screen is already being shown: you should use `requestTransition` instead"};
        }

        m_CurrentScreen = std::move(s);
        m_NextScreen.reset();

        // ensure retained screens are destroyed when exiting this guarded path
        //
        // this means callers can call .show multiple times on the same app
        ScopeGuard const g{[this]()
        {
            m_CurrentScreen.reset();
            m_NextScreen.reset();
        }};

        mainLoopUnguarded();
    }

    void requestTransition(std::unique_ptr<IScreen> s)
    {
        m_NextScreen = std::move(s);
    }

    void requestQuit()
    {
        m_QuitRequested = true;
    }

    Vec2 dims() const
    {
        return Vec2{sdl::GetWindowSize(m_MainWindow.get())};
    }

    void setShowCursor(bool v)
    {
        SDL_ShowCursor(v ? SDL_ENABLE : SDL_DISABLE);
        SDL_SetWindowGrab(m_MainWindow.get(), v ? SDL_FALSE : SDL_TRUE);
    }

    void makeFullscreen()
    {
        SDL_SetWindowFullscreen(m_MainWindow.get(), SDL_WINDOW_FULLSCREEN);
    }

    void makeWindowedFullscreen()
    {
        SDL_SetWindowFullscreen(m_MainWindow.get(), SDL_WINDOW_FULLSCREEN_DESKTOP);
    }

    void makeWindowed()
    {
        SDL_SetWindowFullscreen(m_MainWindow.get(), 0);
    }

    AntiAliasingLevel getCurrentAntiAliasingLevel() const
    {
        return m_CurrentMSXAASamples;
    }

    void setCurrentAntiAliasingLevel(AntiAliasingLevel s)
    {
        m_CurrentMSXAASamples = std::clamp(s, AntiAliasingLevel{1}, getMaxAntiAliasingLevel());
    }

    AntiAliasingLevel getMaxAntiAliasingLevel() const
    {
        return m_GraphicsContext.getMaxAntialiasingLevel();
    }

    bool isInDebugMode() const
    {
        return m_GraphicsContext.isInDebugMode();
    }

    void enableDebugMode()
    {
        m_GraphicsContext.enableDebugMode();
    }

    void disableDebugMode()
    {
        m_GraphicsContext.disableDebugMode();
    }

    bool isVsyncEnabled() const
    {
        return m_GraphicsContext.isVsyncEnabled();
    }

    void setVsync(bool v)
    {
        if (v)
        {
            m_GraphicsContext.enableVsync();
        }
        else
        {
            m_GraphicsContext.disableVsync();
        }
    }

    void enableVsync()
    {
        m_GraphicsContext.enableVsync();
    }

    void disableVsync()
    {
        m_GraphicsContext.disableVsync();
    }

    void addFrameAnnotation(std::string_view label, Rect screenRect)
    {
        m_FrameAnnotations.push_back(ScreenshotAnnotation{std::string{label}, screenRect});
    }

    std::future<Screenshot> requestScreenshot()
    {
        AnnotatedScreenshotRequest& req = m_ActiveAnnotatedScreenshotRequests.emplace_back(m_FrameCounter, requestScreenshotTexture());
        return req.resultPromise.get_future();
    }

    std::string getGraphicsBackendVendorString() const
    {
        return m_GraphicsContext.getBackendVendorString();
    }

    std::string getGraphicsBackendRendererString() const
    {
        return m_GraphicsContext.getBackendRendererString();
    }

    std::string getGraphicsBackendVersionString() const
    {
        return m_GraphicsContext.getBackendVersionString();
    }

    std::string getGraphicsBackendShadingLanguageVersionString() const
    {
        return m_GraphicsContext.getBackendShadingLanguageVersionString();
    }

    uint64_t getFrameCount() const
    {
        return m_FrameCounter;
    }

    AppClock::time_point getAppStartupTime() const
    {
        return m_AppStartupTime;
    }

    AppClock::duration getFrameDeltaSinceAppStartup() const
    {
        return m_FrameStartTime - m_AppStartupTime;
    }

    AppClock::time_point getFrameStartTime() const
    {
        return m_FrameStartTime;
    }

    AppClock::duration getFrameDeltaSinceLastFrame() const
    {
        return m_TimeSinceLastFrame;
    }

    bool isMainLoopWaiting() const
    {
        return m_InWaitMode;
    }

    void setMainLoopWaiting(bool v)
    {
        m_InWaitMode = v;
        requestRedraw();
    }

    void makeMainEventLoopWaiting()
    {
        setMainLoopWaiting(true);
    }

    void makeMainEventLoopPolling()
    {
        setMainLoopWaiting(false);
    }

    void requestRedraw()
    {
        SDL_Event e{};
        e.type = SDL_USEREVENT;
        m_NumFramesToPoll += 2;  // immediate rendering can require rendering 2 frames before it shows something
        SDL_PushEvent(&e);
    }

    void clearScreen(Color const& color)
    {
        m_GraphicsContext.clearScreen(color);
    }

    void setMainWindowSubTitle(std::string_view sv)
    {
        // use global + mutex to prevent hopping into the OS too much
        static SynchronizedValue<std::string> s_CurrentWindowSubTitle;

        auto lock = s_CurrentWindowSubTitle.lock();

        if (sv == *lock)
        {
            return;
        }

        *lock = sv;

        std::string const newTitle = sv.empty() ?
            std::string{GetBestHumanReadableApplicationName(m_Metadata)} :
            (std::string{sv} + " - " + GetBestHumanReadableApplicationName(m_Metadata));

        SDL_SetWindowTitle(m_MainWindow.get(), newTitle.c_str());
    }

    void unsetMainWindowSubTitle()
    {
        setMainWindowSubTitle({});
    }

    AppConfig const& getConfig() const
    {
        return *m_ApplicationConfig;
    }

    AppConfig& updConfig()
    {
        return *m_ApplicationConfig;
    }

    ResourceLoader const& getResourceLoader() const
    {
        return m_AppResourceLoader;
    }

    std::filesystem::path getResourceFilepath(ResourcePath const& rp) const
    {
        return std::filesystem::weakly_canonical(m_ApplicationConfig->getResourceDir() / rp.string());
    }

    std::string slurpResource(ResourcePath const& rp)
    {
        return m_AppResourceLoader.slurp(rp);
    }

    ResourceStream loadResource(ResourcePath const& rp)
    {
        return m_AppResourceLoader.open(rp);
    }

    std::shared_ptr<void> updSingleton(
        std::type_info const& typeinfo,
        std::function<std::shared_ptr<void>()> const& ctor)
    {
        auto lock = m_Singletons.lock();
        auto const [it, inserted] = lock->try_emplace(TypeInfoReference{typeinfo}, nullptr);
        if (inserted)
        {
            it->second = ctor();
        }
        return it->second;
    }

    sdl::Window& updWindow()
    {
        return m_MainWindow;
    }

    GraphicsContext& updGraphicsContext()
    {
        return m_GraphicsContext;
    }

    void* updRawGLContextHandleHACK()
    {
        return m_GraphicsContext.updRawGLContextHandleHACK();
    }

private:
    bool isWindowFocused() const
    {
        return (SDL_GetWindowFlags(m_MainWindow.get()) & SDL_WINDOW_INPUT_FOCUS) != 0u;
    }

    std::future<Texture2D> requestScreenshotTexture()
    {
        return m_GraphicsContext.requestScreenshot();
    }

    // perform a screen transntion between two top-level `IScreen`s
    void transitionToNextScreen()
    {
        if (!m_NextScreen)
        {
            return;
        }

        log_info("unmounting screen %s", m_CurrentScreen->getName().c_str());

        try
        {
            m_CurrentScreen->onUnmount();
        }
        catch (std::exception const& ex)
        {
            log_error("error unmounting screen %s: %s", m_CurrentScreen->getName().c_str(), ex.what());
            m_CurrentScreen.reset();
            throw;
        }

        m_CurrentScreen.reset();
        m_CurrentScreen = std::move(m_NextScreen);

        // the next screen might need to draw a couple of frames
        // to "warm up" (e.g. because it's using an immediate ui)
        m_NumFramesToPoll = 2;

        log_info("mounting screen %s", m_CurrentScreen->getName().c_str());
        m_CurrentScreen->onMount();
        log_info("transitioned main screen to %s", m_CurrentScreen->getName().c_str());
    }

    // the main application loop
    //
    // this is what he application enters when it `show`s the first screen
    void mainLoopUnguarded()
    {
        // perform initial screen mount
        m_CurrentScreen->onMount();

        // ensure current screen is unmounted and the quitting flag is reset when
        // exiting the main loop
        ScopeGuard const onQuitGuard{[this]()
        {
            if (m_CurrentScreen) {
                m_CurrentScreen->onUnmount();
            }
            m_QuitRequested = false;
        }};

        // reset counters
        m_AppCounter = SDL_GetPerformanceCounter();
        m_FrameCounter = 0;
        m_FrameStartTime = ConvertPerfCounterToFClock(m_AppCounter, m_AppCounterFq);
        m_TimeSinceLastFrame = AppClock::duration{1.0f/60.0f};  // (estimated value for first frame)

        while (true)  // gameloop
        {
            // pump events
            {
                OSC_PERF("App/pumpEvents");

                bool shouldWait = m_InWaitMode && m_NumFramesToPoll <= 0;
                m_NumFramesToPoll = std::max(0, m_NumFramesToPoll - 1);

                for (SDL_Event e; shouldWait ? SDL_WaitEventTimeout(&e, 1000) : SDL_PollEvent(&e);)
                {
                    shouldWait = false;

                    if (e.type == SDL_WINDOWEVENT)
                    {
                        // window was resized and should be drawn a couple of times quickly
                        // to ensure any immediate UIs in screens are updated
                        m_NumFramesToPoll = 2;
                    }

                    // let screen handle the event
                    m_CurrentScreen->onEvent(e);

                    if (m_QuitRequested)
                    {
                        // screen requested application quit, so exit this function
                        return;
                    }

                    if (m_NextScreen)
                    {
                        // screen requested a new screen, so perform the transition
                        transitionToNextScreen();
                    }

                    if (e.type == SDL_DROPTEXT || e.type == SDL_DROPFILE)
                    {
                        SDL_free(e.drop.file);  // SDL documentation mandates that the caller frees this
                    }
                }
            }

            // update clocks
            {
                auto counter = SDL_GetPerformanceCounter();

                Uint64 deltaTicks = counter - m_AppCounter;

                m_AppCounter = counter;
                m_FrameStartTime = ConvertPerfCounterToFClock(counter, m_AppCounterFq);
                m_TimeSinceLastFrame = ConvertPerfTicksToFClockDuration(deltaTicks, m_AppCounterFq);
            }

            // "tick" the screen
            {
                OSC_PERF("App/onTick");
                m_CurrentScreen->onTick();
            }

            if (m_QuitRequested)
            {
                // screen requested application quit, so exit this function
                return;
            }

            if (m_NextScreen)
            {
                // screen requested a new screen, so perform the transition
                transitionToNextScreen();
                continue;
            }

            // "draw" the screen into the window framebuffer
            {
                OSC_PERF("App/onDraw");
                m_CurrentScreen->onDraw();
            }

            // "present" the rendered screen to the user (can block on VSYNC)
            {
                OSC_PERF("App/doSwapBuffers");
                m_GraphicsContext.doSwapBuffers(*m_MainWindow);
            }

            // handle annotated screenshot requests (if any)
            {
                // save this frame's annotations into the requests, if necessary
                for (AnnotatedScreenshotRequest& req : m_ActiveAnnotatedScreenshotRequests)
                {
                    if (req.frameRequested == m_FrameCounter)
                    {
                        req.annotations = m_FrameAnnotations;
                    }
                }
                m_FrameAnnotations.clear();  // this frame's annotations are now saved (if necessary)

                // complete any requests for which screenshot data has arrived
                for (AnnotatedScreenshotRequest& req : m_ActiveAnnotatedScreenshotRequests)
                {
                    if (req.underlyingScreenshotFuture.valid() &&
                        req.underlyingScreenshotFuture.wait_for(std::chrono::seconds{0}) == std::future_status::ready)
                    {
                        // screenshot is ready: create an annotated screenshot and send it to
                        // the caller
                        req.resultPromise.set_value(Screenshot{req.underlyingScreenshotFuture.get(), std::move(req.annotations)});
                    }
                }

                // gc any invalid (i.e. handled) requests
                std::erase_if(
                    m_ActiveAnnotatedScreenshotRequests,
                    [](AnnotatedScreenshotRequest const& req)
                    {
                        return !req.underlyingScreenshotFuture.valid();
                    }
                );
            }

            // care: only update the frame counter here because the above methods
            // and checks depend on it being consistient throughout a single crank
            // of the application loop
            ++m_FrameCounter;

            if (m_QuitRequested)
            {
                // screen requested application quit, so exit this function
                return;
            }

            if (m_NextScreen)
            {
                // screen requested a new screen, so perform the transition
                transitionToNextScreen();
                continue;
            }
        }
    }

    // immutable application metadata (can be provided at runtime via ctor)
    AppMetadata m_Metadata;

    // path to the directory that the application's executable is contained within
    std::filesystem::path m_ExecutableDirPath = GetCurrentExeDirAndLogIt();

    // path to the write-able user data directory
    std::filesystem::path m_UserDataDirPath = GetUserDataDirAndLogIt(
        m_Metadata.getOrganizationName(),
        m_Metadata.getApplicationName()
    );

    // top-level application configuration
    std::shared_ptr<AppConfig> m_ApplicationConfig = std::make_shared<AppConfig>(
        m_Metadata.getOrganizationName(),
        m_Metadata.getApplicationName()
    );

    // ensure the application log is configured according to the given configuration file
    bool m_ApplicationLogIsConfigured = ConfigureApplicationLog(*m_ApplicationConfig);

    // enable the stack backtrace handler (if necessary - once per process)
    bool m_IsBacktraceHandlerInstalled = EnsureBacktraceHandlerEnabled(m_UserDataDirPath);

    // top-level runtime resource loader
    ResourceLoader m_AppResourceLoader = make_resource_loader<AppResourceLoader>(m_ApplicationConfig);

    // init SDL context (windowing, etc.)
    sdl::Context m_SDLContext{SDL_INIT_VIDEO};

    // init main application window
    sdl::Window m_MainWindow = CreateMainAppWindow(GetBestHumanReadableApplicationName(m_Metadata));

    // init graphics context
    GraphicsContext m_GraphicsContext{*m_MainWindow};

    // get performance counter frequency (for the delta clocks)
    Uint64 m_AppCounterFq = SDL_GetPerformanceFrequency();

    // current performance counter value (recorded once per frame)
    Uint64 m_AppCounter = 0;

    // number of frames the application has drawn
    uint64_t m_FrameCounter = 0;

    // when the application started up (set now)
    AppClock::time_point m_AppStartupTime = ConvertPerfCounterToFClock(SDL_GetPerformanceCounter(), m_AppCounterFq);

    // when the current frame started (set each frame)
    AppClock::time_point m_FrameStartTime = m_AppStartupTime;

    // time since the frame before the current frame (set each frame)
    AppClock::duration m_TimeSinceLastFrame = {};

    // global cache of application-wide singletons (usually, for caching)
    SynchronizedValue<std::unordered_map<TypeInfoReference, std::shared_ptr<void>>> m_Singletons;

    // how many antiAliasingLevel the implementation should actually use
    AntiAliasingLevel m_CurrentMSXAASamples = std::min(m_GraphicsContext.getMaxAntialiasingLevel(), m_ApplicationConfig->getNumMSXAASamples());

    // set to true if the application should quit
    bool m_QuitRequested = false;

    // set to true if the main loop should pause on events
    //
    // CAREFUL: this makes the app event-driven
    bool m_InWaitMode = false;

    // set >0 to force that `n` frames are polling-driven: even in waiting mode
    int32_t m_NumFramesToPoll = 0;

    // current screen being shown (if any)
    std::unique_ptr<IScreen> m_CurrentScreen;

    // the *next* screen the application should show
    std::unique_ptr<IScreen> m_NextScreen;

    // frame annotations made during this frame
    std::vector<ScreenshotAnnotation> m_FrameAnnotations;

    // any active promises for an annotated frame
    std::vector<AnnotatedScreenshotRequest> m_ActiveAnnotatedScreenshotRequests;
};

// public API

App& osc::App::upd()
{
    OSC_ASSERT(g_ApplicationGlobal && "App is not initialized: have you constructed a (singleton) instance of App?");
    return *g_ApplicationGlobal;
}

App const& osc::App::get()
{
    OSC_ASSERT(g_ApplicationGlobal && "App is not initialized: have you constructed a (singleton) instance of App?");
    return *g_ApplicationGlobal;
}

AppConfig const& osc::App::config()
{
    return get().getConfig();
}

std::filesystem::path osc::App::resourceFilepath(ResourcePath const& rp)
{
    return get().getResourceFilepath(rp);
}

std::string osc::App::slurp(ResourcePath const& rp)
{
    return upd().slurpResource(rp);
}

ResourceStream osc::App::load_resource(ResourcePath const& rp)
{
    return upd().loadResource(rp);
}

ResourceLoader const& osc::App::resource_loader()
{
    return upd().getResourceLoader();
}

osc::App::App() :
    App{AppMetadata{}}
{
}

osc::App::App(AppMetadata const& metadata)
{
    OSC_ASSERT(!g_ApplicationGlobal && "cannot instantiate multiple `App` instances at the same time");

    m_Impl = std::make_unique<Impl>(metadata);
    g_ApplicationGlobal = this;
}

osc::App::App(App&&) noexcept = default;

App& osc::App::operator=(App&&) noexcept = default;

osc::App::~App() noexcept
{
    g_ApplicationGlobal = nullptr;
}

AppMetadata const& osc::App::getMetadata() const
{
    return m_Impl->getMetadata();
}

std::filesystem::path const& osc::App::getExecutableDirPath() const
{
    return m_Impl->getExecutableDirPath();
}

std::filesystem::path const& osc::App::getUserDataDirPath() const
{
    return m_Impl->getUserDataDirPath();
}

void osc::App::show(std::unique_ptr<IScreen> s)
{
    m_Impl->show(std::move(s));
}

void osc::App::requestTransition(std::unique_ptr<IScreen> s)
{
    m_Impl->requestTransition(std::move(s));
}

void osc::App::requestQuit()
{
    m_Impl->requestQuit();
}

Vec2 osc::App::dims() const
{
    return m_Impl->dims();
}

void osc::App::setShowCursor(bool v)
{
    m_Impl->setShowCursor(v);
}

void osc::App::makeFullscreen()
{
    m_Impl->makeFullscreen();
}

void osc::App::makeWindowedFullscreen()
{
    m_Impl->makeWindowedFullscreen();
}

void osc::App::makeWindowed()
{
    m_Impl->makeWindowed();
}

AntiAliasingLevel osc::App::getCurrentAntiAliasingLevel() const
{
    return m_Impl->getCurrentAntiAliasingLevel();
}

void osc::App::setCurrentAntiAliasingLevel(AntiAliasingLevel s)
{
    m_Impl->setCurrentAntiAliasingLevel(s);
}

AntiAliasingLevel osc::App::getMaxAntiAliasingLevel() const
{
    return m_Impl->getMaxAntiAliasingLevel();
}

bool osc::App::isInDebugMode() const
{
    return m_Impl->isInDebugMode();
}

void osc::App::enableDebugMode()
{
    m_Impl->enableDebugMode();
}

void osc::App::disableDebugMode()
{
    m_Impl->disableDebugMode();
}

bool osc::App::isVsyncEnabled() const
{
    return m_Impl->isVsyncEnabled();
}

void osc::App::setVsync(bool v)
{
    m_Impl->setVsync(v);
}

void osc::App::enableVsync()
{
    m_Impl->enableVsync();
}

void osc::App::disableVsync()
{
    m_Impl->disableVsync();
}

void osc::App::addFrameAnnotation(std::string_view label, Rect screenRect)
{
    m_Impl->addFrameAnnotation(label, screenRect);
}

std::future<Screenshot> osc::App::requestScreenshot()
{
    return m_Impl->requestScreenshot();
}

std::string osc::App::getGraphicsBackendVendorString() const
{
    return m_Impl->getGraphicsBackendVendorString();
}

std::string osc::App::getGraphicsBackendRendererString() const
{
    return m_Impl->getGraphicsBackendRendererString();
}

std::string osc::App::getGraphicsBackendVersionString() const
{
    return m_Impl->getGraphicsBackendVersionString();
}

std::string osc::App::getGraphicsBackendShadingLanguageVersionString() const
{
    return m_Impl->getGraphicsBackendShadingLanguageVersionString();
}

uint64_t osc::App::getFrameCount() const
{
    return m_Impl->getFrameCount();
}

AppClock::time_point osc::App::getAppStartupTime() const
{
    return m_Impl->getAppStartupTime();
}

AppClock::duration osc::App::getFrameDeltaSinceAppStartup() const
{
    return m_Impl->getFrameDeltaSinceAppStartup();
}

AppClock::time_point osc::App::getFrameStartTime() const
{
    return m_Impl->getFrameStartTime();
}

AppClock::duration osc::App::getFrameDeltaSinceLastFrame() const
{
    return m_Impl->getFrameDeltaSinceLastFrame();
}

bool osc::App::isMainLoopWaiting() const
{
    return m_Impl->isMainLoopWaiting();
}

void osc::App::setMainLoopWaiting(bool v)
{
    m_Impl->setMainLoopWaiting(v);
}

void osc::App::makeMainEventLoopWaiting()
{
    m_Impl->makeMainEventLoopWaiting();
}

void osc::App::makeMainEventLoopPolling()
{
    m_Impl->makeMainEventLoopPolling();
}

void osc::App::requestRedraw()
{
    m_Impl->requestRedraw();
}

void osc::App::clearScreen(Color const& color)
{
    m_Impl->clearScreen(color);
}

void osc::App::setMainWindowSubTitle(std::string_view sv)
{
    m_Impl->setMainWindowSubTitle(sv);
}

void osc::App::unsetMainWindowSubTitle()
{
    m_Impl->unsetMainWindowSubTitle();
}

AppConfig const& osc::App::getConfig() const
{
    return m_Impl->getConfig();
}

AppConfig& osc::App::updConfig()
{
    return m_Impl->updConfig();
}

ResourceLoader const& osc::App::getResourceLoader() const
{
    return m_Impl->getResourceLoader();
}

std::filesystem::path osc::App::getResourceFilepath(ResourcePath const& rp) const
{
    return m_Impl->getResourceFilepath(rp);
}

std::string osc::App::slurpResource(ResourcePath const& rp)
{
    return m_Impl->slurpResource(rp);
}

ResourceStream osc::App::loadResource(ResourcePath const& rp)
{
    return m_Impl->loadResource(rp);
}

std::shared_ptr<void> osc::App::updSingleton(std::type_info const& typeInfo, std::function<std::shared_ptr<void>()> const& ctor)
{
    return m_Impl->updSingleton(typeInfo, ctor);
}

SDL_Window* osc::App::updUndleryingWindow()
{
    return m_Impl->updWindow().get();
}

void* osc::App::updUnderlyingOpenGLContext()
{
    return m_Impl->updGraphicsContext().updRawGLContextHandleHACK();
}

#include "App.hpp"

#include <oscar/Bindings/SDL2Helpers.hpp>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/GraphicsContext.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Platform/AppClock.hpp>
#include <oscar/Platform/AppConfig.hpp>
#include <oscar/Platform/AppMetadata.hpp>
#include <oscar/Platform/Log.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Platform/Screen.hpp>
#include <oscar/Platform/Screenshot.hpp>
#include <oscar/Utils/Cpp20Shims.hpp>
#include <oscar/Utils/FilesystemHelpers.hpp>
#include <oscar/Utils/Perf.hpp>
#include <oscar/Utils/ScopeGuard.hpp>
#include <oscar/Utils/StringHelpers.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <glm/vec2.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl2.h>
#include <SDL.h>
#include <SDL_error.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>
#include <SDL_video.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <ctime>
#include <exception>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace
{
    osc::App* g_ApplicationGlobal = nullptr;

    constexpr auto c_IconRanges = osc::to_array<ImWchar>({ ICON_MIN_FA, ICON_MAX_FA, 0 });

    void Sdl_GL_SetAttributeOrThrow(
        SDL_GLattr attr,
        osc::CStringView attrReadableName,
        int value,
        osc::CStringView valueReadableName)
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
        osc::log::info("enabling backtrace handler");
        osc::InstallBacktraceHandler(crashDumpDir);
        return true;
    }

    bool ConfigureApplicationLog(osc::AppConfig const& config)
    {
        if (auto logger = osc::log::defaultLogger())
        {
            logger->set_level(config.getRequestedLogLevel());
        }
        return true;
    }

    // returns a resource from the config-provided `resources/` dir
    std::filesystem::path GetResource(osc::AppConfig const& c, std::string_view p)
    {
        return c.getResourceDir() / p;
    }

    // initialize the main application window
    sdl::Window CreateMainAppWindow(osc::CStringView applicationName)
    {
        osc::log::info("initializing main application window");

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

    osc::AppClock::duration ConvertPerfTicksToFClockDuration(Uint64 ticks, Uint64 frequency)
    {
        auto const dticks = static_cast<double>(ticks);
        auto const dfrequency = static_cast<double>(frequency);
        auto const duration = static_cast<osc::AppClock::rep>(dticks/dfrequency);

        return osc::AppClock::duration{duration};
    }

    osc::AppClock::time_point ConvertPerfCounterToFClock(Uint64 ticks, Uint64 frequency)
    {
        return osc::AppClock::time_point{ConvertPerfTicksToFClockDuration(ticks, frequency)};
    }
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
            std::future<osc::Texture2D> underlyingFuture_) :

            frameRequested{frameRequested_},
            underlyingScreenshotFuture{std::move(underlyingFuture_)}
        {
        }

        // the frame on which the screenshot was requested
        uint64_t frameRequested;

        // underlying (to-be-waited-on) future for the screenshot
        std::future<osc::Texture2D> underlyingScreenshotFuture;

        // our promise to the caller, who is waiting for an annotated image
        std::promise<osc::Screenshot> resultPromise;

        // annotations made during the requested frame (if any)
        std::vector<osc::ScreenshotAnnotation> annotations;
    };

    // wrapper class for storing std::type_info as a hashable type
    class TypeInfoReference {
    public:
        explicit TypeInfoReference(std::type_info const& typeInfo) noexcept :
            m_TypeInfo{&typeInfo}
        {
        }

        std::type_info const& get() const
        {
            return *m_TypeInfo;
        }
    private:
        std::type_info const* m_TypeInfo;
    };

    bool operator==(TypeInfoReference const& a, TypeInfoReference const& b) noexcept
    {
        return a.get() == b.get();
    }
}

namespace std
{
    template<>
    struct hash<TypeInfoReference> final {
        size_t operator()(TypeInfoReference const& ref) const noexcept
        {
            return ref.get().hash_code();
        }
    };
}

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

    void show(std::unique_ptr<Screen> s)
    {
        log::info("showing screen %s", s->getName().c_str());

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

    void requestTransition(std::unique_ptr<Screen> s)
    {
        m_NextScreen = std::move(s);
    }

    void requestQuit()
    {
        m_QuitRequested = true;
    }

    glm::vec2 dims() const
    {
        return glm::vec2{sdl::GetWindowSize(m_MainWindow.get())};
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
        m_NumFramesToPoll += 2;  // some parts of ImGui require rendering 2 frames before it shows something
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
            std::string{osc::GetBestHumanReadableApplicationName(m_Metadata)} :
            (std::string{sv} + " - " + osc::GetBestHumanReadableApplicationName(m_Metadata));

        SDL_SetWindowTitle(m_MainWindow.get(), newTitle.c_str());
    }

    void unsetMainWindowSubTitle()
    {
        setMainWindowSubTitle({});
    }

    osc::AppConfig const& getConfig() const
    {
        return m_ApplicationConfig;
    }

    AppConfig& updConfig()
    {
        return m_ApplicationConfig;
    }

    std::filesystem::path getResource(std::string_view p) const
    {
        return ::GetResource(m_ApplicationConfig, p);
    }

    std::string slurpResource(std::string_view p) const
    {
        std::filesystem::path path = getResource(p);
        return SlurpFileIntoString(path);
    }

    std::shared_ptr<void> updSingleton(std::type_info const& typeinfo, std::function<std::shared_ptr<void>()> const& ctor)
    {
        auto lock = m_Singletons.lock();
        auto const [it, inserted] = lock->try_emplace(TypeInfoReference{typeinfo}, nullptr);
        if (inserted)
        {
            it->second = ctor();
        }
        return it->second;
    }

    // used by ImGui backends

    sdl::Window& updWindow()
    {
        return m_MainWindow;
    }

    osc::GraphicsContext& updGraphicsContext()
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

    // perform a screen transntion between two top-level `osc::Screen`s
    void transitionToNextScreen()
    {
        if (!m_NextScreen)
        {
            return;
        }

        log::info("unmounting screen %s", m_CurrentScreen->getName().c_str());

        try
        {
            m_CurrentScreen->onUnmount();
        }
        catch (std::exception const& ex)
        {
            log::error("error unmounting screen %s: %s", m_CurrentScreen->getName().c_str(), ex.what());
            m_CurrentScreen.reset();
            throw;
        }

        m_CurrentScreen.reset();
        m_CurrentScreen = std::move(m_NextScreen);

        // the next screen might need to draw a couple of frames
        // to "warm up" (e.g. because it's using ImGui)
        m_NumFramesToPoll = 2;

        log::info("mounting screen %s", m_CurrentScreen->getName().c_str());
        m_CurrentScreen->onMount();
        log::info("transitioned main screen to %s", m_CurrentScreen->getName().c_str());
    }

    // the main application loop
    //
    // this is what he application enters when it `show`s the first screen
    void mainLoopUnguarded()
    {
        // perform initial screen mount
        m_CurrentScreen->onMount();

        // ensure current screen is unmounted when exiting the main loop
        ScopeGuard const g{[this]() { if (m_CurrentScreen) { m_CurrentScreen->onUnmount(); }}};

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
                        // to ensure any datastructures in the screens (namely: imgui) are
                        // updated
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
                osc::erase_if(
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

    // provided via the constructor
    AppMetadata m_Metadata;

    // path to the directory that the executable is contained within
    std::filesystem::path m_ExecutableDirPath = osc::CurrentExeDir();

    // compute where a write-able user data dir is
    std::filesystem::path m_UserDataDirPath = osc::GetUserDataDir(
        m_Metadata.getOrganizationName(),
        m_Metadata.getApplicationName()
    );

    // init/load the application config first
    AppConfig m_ApplicationConfig
    {
        m_Metadata.getOrganizationName(),
        m_Metadata.getApplicationName(),
    };

    // ensure the application log is configured according to the given configuration file
    bool m_ApplicationLogIsConfigured = ConfigureApplicationLog(m_ApplicationConfig);

    // install the backtrace handler (if necessary - once per process)
    bool m_IsBacktraceHandlerInstalled = EnsureBacktraceHandlerEnabled(m_UserDataDirPath);

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
    AntiAliasingLevel m_CurrentMSXAASamples = std::min(m_GraphicsContext.getMaxAntialiasingLevel(), m_ApplicationConfig.getNumMSXAASamples());

    // set to true if the application should quit
    bool m_QuitRequested = false;

    // set to true if the main loop should pause on events
    //
    // CAREFUL: this makes the app event-driven
    bool m_InWaitMode = false;

    // set >0 to force that `n` frames are polling-driven: even in waiting mode
    int32_t m_NumFramesToPoll = 0;

    // current screen being shown (if any)
    std::unique_ptr<Screen> m_CurrentScreen;

    // the *next* screen the application should show
    std::unique_ptr<Screen> m_NextScreen;

    // frame annotations made during this frame
    std::vector<ScreenshotAnnotation> m_FrameAnnotations;

    // any active promises for an annotated frame
    std::vector<AnnotatedScreenshotRequest> m_ActiveAnnotatedScreenshotRequests;
};

// public API

osc::App& osc::App::upd()
{
    OSC_ASSERT(g_ApplicationGlobal && "App is not initialized: have you constructed a (singleton) instance of App?");
    return *g_ApplicationGlobal;
}

osc::App const& osc::App::get()
{
    OSC_ASSERT(g_ApplicationGlobal && "App is not initialized: have you constructed a (singleton) instance of App?");
    return *g_ApplicationGlobal;
}

osc::AppConfig const& osc::App::config()
{
    return get().getConfig();
}

std::filesystem::path osc::App::resource(std::string_view s)
{
    return get().getResource(s);
}

std::string osc::App::slurp(std::string_view s)
{
    return get().slurpResource(s);
}

osc::App::App(AppMetadata const& metadata)
{
    OSC_ASSERT(!g_ApplicationGlobal && "cannot instantiate multiple osc::App instances at the same time");

    m_Impl = std::make_unique<Impl>(metadata);
    g_ApplicationGlobal = this;
}

osc::App::App(App&&) noexcept = default;

osc::App& osc::App::operator=(App&&) noexcept = default;

osc::App::~App() noexcept
{
    g_ApplicationGlobal = nullptr;
}

osc::AppMetadata const& osc::App::getMetadata() const
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

void osc::App::show(std::unique_ptr<Screen> s)
{
    m_Impl->show(std::move(s));
}

void osc::App::requestTransition(std::unique_ptr<Screen> s)
{
    m_Impl->requestTransition(std::move(s));
}

void osc::App::requestQuit()
{
    m_Impl->requestQuit();
}

glm::vec2 osc::App::dims() const
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

osc::AntiAliasingLevel osc::App::getCurrentAntiAliasingLevel() const
{
    return m_Impl->getCurrentAntiAliasingLevel();
}

void osc::App::setCurrentAntiAliasingLevel(osc::AntiAliasingLevel s)
{
    m_Impl->setCurrentAntiAliasingLevel(s);
}

osc::AntiAliasingLevel osc::App::getMaxAntiAliasingLevel() const
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

std::future<osc::Screenshot> osc::App::requestScreenshot()
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

osc::AppClock::time_point osc::App::getAppStartupTime() const
{
    return m_Impl->getAppStartupTime();
}

osc::AppClock::duration osc::App::getFrameDeltaSinceAppStartup() const
{
    return m_Impl->getFrameDeltaSinceAppStartup();
}

osc::AppClock::time_point osc::App::getFrameStartTime() const
{
    return m_Impl->getFrameStartTime();
}

osc::AppClock::duration osc::App::getFrameDeltaSinceLastFrame() const
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

osc::AppConfig const& osc::App::getConfig() const
{
    return m_Impl->getConfig();
}

osc::AppConfig& osc::App::updConfig()
{
    return m_Impl->updConfig();
}

std::filesystem::path osc::App::getResource(std::string_view p) const
{
    return m_Impl->getResource(p);
}

std::string osc::App::slurpResource(std::string_view p) const
{
    return m_Impl->slurpResource(p);
}

std::shared_ptr<void> osc::App::updSingleton(std::type_info const& typeInfo, std::function<std::shared_ptr<void>()> const& ctor)
{
    return m_Impl->updSingleton(typeInfo, ctor);
}

void osc::ImGuiInit()
{
    // init ImGui top-level context
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    // configure ImGui from OSC's (toml) configuration
    {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        if (App::get().getConfig().isMultiViewportEnabled())
        {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        }
    }

    // make it so that windows can only ever be moved from the title bar
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;

    // load application-level ImGui config, then the user one,
    // so that the user config takes precedence
    {
        std::string defaultIni = App::resource("imgui_base_config.ini").string();
        ImGui::LoadIniSettingsFromDisk(defaultIni.c_str());

        // CARE: the reason this filepath is `static` is because ImGui requires that
        // the string outlives the ImGui context
        static std::string const s_UserImguiIniFilePath = (App::get().getUserDataDirPath() / "imgui.ini").string();

        ImGui::LoadIniSettingsFromDisk(s_UserImguiIniFilePath.c_str());
        io.IniFilename = s_UserImguiIniFilePath.c_str();
    }

    ImFontConfig baseConfig;
    baseConfig.SizePixels = 15.0f;
    baseConfig.PixelSnapH = true;
    baseConfig.OversampleH = 2;
    baseConfig.OversampleV = 2;
    std::string baseFontFile = App::resource("oscar/fonts/Ruda-Bold.ttf").string();
    io.Fonts->AddFontFromFileTTF(baseFontFile.c_str(), baseConfig.SizePixels, &baseConfig);

    // add FontAwesome icon support
    {
        ImFontConfig config = baseConfig;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = std::floor(1.5f * config.SizePixels);
        config.GlyphMaxAdvanceX = std::floor(1.5f * config.SizePixels);

        std::string const fontFile = App::resource("oscar/fonts/fa-solid-900.ttf").string();
        io.Fonts->AddFontFromFileTTF(
            fontFile.c_str(),
            config.SizePixels,
            &config,
            c_IconRanges.data()
        );
    }

    // init ImGui for SDL2 /w OpenGL
    App::Impl& impl = *App::upd().m_Impl;
    ImGui_ImplSDL2_InitForOpenGL(impl.updWindow().get(), impl.updRawGLContextHandleHACK());

    // init ImGui for OpenGL
    ImGui_ImplOpenGL3_Init("#version 330 core");

    ImGuiApplyDarkTheme();
}

void osc::ImGuiShutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool osc::ImGuiOnEvent(SDL_Event const& e)
{
    ImGui_ImplSDL2_ProcessEvent(&e);

    ImGuiIO const& io  = ImGui::GetIO();

    bool handledByImgui = false;

    if (io.WantCaptureKeyboard && (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP))
    {
        handledByImgui = true;
    }

    if (io.WantCaptureMouse && (e.type == SDL_MOUSEWHEEL || e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEBUTTONDOWN))
    {
        handledByImgui = true;
    }

    return handledByImgui;
}

void osc::ImGuiNewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(App::upd().m_Impl->updWindow().get());
    ImGui::NewFrame();
}

void osc::ImGuiRender()
{
    {
        OSC_PERF("ImGuiRender/Render");
        ImGui::Render();
    }

    {
        OSC_PERF("ImGuiRender/ImGui_ImplOpenGL3_RenderDrawData");

        // HACK: convert all ImGui-provided colors from sRGB to linear
        //
        // this is necessary because the ImGui OpenGL backend's shaders
        // assume all color vertices and colors from textures are in
        // sRGB, but OSC can provide ImGui with linear OR sRGB textures
        // because OSC assumes the OpenGL backend is using automatic
        // color conversion support (in ImGui, it isn't)
        //
        // so what we do here is linearize all colors from ImGui and
        // always provide textures in the OSC style. The shaders in ImGui
        // then write linear color values to the screen, but because we
        // are *also* enabling GL_FRAMEBUFFER_SRGB, the OpenGL backend
        // will correctly convert those linear colors to sRGB if necessary
        // automatically
        //
        // (this shitshow is because ImGui's OpenGL backend behaves differently
        //  from OSCs - ultimately, we need an ImGui_ImplOSC backend)
        ConvertDrawDataFromSRGBToLinear(*ImGui::GetDrawData());
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // ImGui: handle multi-viewports if the user has requested them
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backupCurrentWindow = SDL_GL_GetCurrentWindow();
        SDL_GLContext backupCurrentContext = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backupCurrentWindow, backupCurrentContext);
    }
}

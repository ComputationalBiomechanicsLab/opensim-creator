#include "App.hpp"

#include "osc_config.hpp"

#include "src/Bindings/SDL2Helpers.hpp"
#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/GraphicsContext.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Platform/Config.hpp"
#include "src/Platform/Log.hpp"
#include "src/Platform/os.hpp"
#include "src/Platform/RecentFile.hpp"
#include "src/Platform/Screen.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/FilesystemHelpers.hpp"
#include "src/Utils/ScopeGuard.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <IconsFontAwesome5.h>
#include <imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>
#include <SDL.h>
#include <SDL_error.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>
#include <SDL_video.h>

#include <algorithm>
#include <cstring>
#include <cmath>
#include <ctime>
#include <exception>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>


// install backtrace dumper
//
// useful if the application fails in prod: can provide some basic backtrace
// info that users can paste into an issue or something, which is *a lot* more
// information than "yeah, it's broke"
static bool EnsureBacktraceHandlerEnabled()
{
    static bool enabledOnceGlobally = []()
    {
        osc::log::info("enabling backtrace handler");
        osc::InstallBacktraceHandler();
        return true;
    }();

    return enabledOnceGlobally;
}

// returns a resource from the config-provided `resources/` dir
static std::filesystem::path GetResource(osc::Config const& c, std::string_view p)
{
    return c.getResourceDir() / p;
}

// handy macro for calling SDL_GL_SetAttribute with error checking
#define OSC_SDL_GL_SetAttribute_CHECK(attr, value)                                                                     \
    {                                                                                                                  \
        int rv = SDL_GL_SetAttribute((attr), (value));                                                                 \
        if (rv != 0) {                                                                                                 \
            throw std::runtime_error{std::string{"SDL_GL_SetAttribute failed when setting " #attr " = " #value " : "} +            \
                                     SDL_GetError()};                                                                  \
        }                                                                                                              \
    }

static char const* g_BaseWindowTitle = "OpenSim Creator v" OSC_VERSION_STRING;

// initialize the main application window
static sdl::Window CreateMainAppWindow()
{
    osc::log::info("initializing main application (OpenGL 3.3) window");

    OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

    // careful about setting resolution, position, etc. - some people have *very* shitty
    // screens on their laptop (e.g. ultrawide, sub-HD, minus space for the start bar, can
    // be <700 px high)
    constexpr int x = SDL_WINDOWPOS_CENTERED;
    constexpr int y = SDL_WINDOWPOS_CENTERED;
    constexpr int width = 800;
    constexpr int height = 600;
    constexpr Uint32 flags =
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;

    return sdl::CreateWindoww(g_BaseWindowTitle, x, y, width, height, flags);
}

// returns refresh rate of highest refresh rate display on the computer
static int GetHighestRefreshRateDisplay()
{
    int numDisplays = SDL_GetNumVideoDisplays();

    if (numDisplays < 1)
    {
        return 60;  // this should be impossible but, you know, coding.
    }

    int highestRefreshRate = 30;
    SDL_DisplayMode modeStruct{};
    for (int display = 0; display < numDisplays; ++display)
    {
        int numModes = SDL_GetNumDisplayModes(display);
        for (int mode = 0; mode < numModes; ++mode)
        {
            SDL_GetDisplayMode(display, mode, &modeStruct);
            highestRefreshRate = std::max(highestRefreshRate, modeStruct.refresh_rate);
        }
    }
    return highestRefreshRate;
}

// load the "recent files" file that osc persists to disk
static std::vector<osc::RecentFile> LoadRecentFilesFile(std::filesystem::path const& p)
{
    std::ifstream fd{p, std::ios::in};

    if (!fd)
    {
        // do not throw, because it probably shouldn't crash the application if this
        // is an issue
        osc::log::error("%s: could not be opened for reading: cannot load recent files list", p.string().c_str());
        return {};
    }

    std::vector<osc::RecentFile> rv;
    std::string line;

    while (std::getline(fd, line))
    {
        std::istringstream ss{line};

        // read line content
        uint64_t timestamp;
        std::filesystem::path path;
        ss >> timestamp;
        ss >> path;

        // calc tertiary data
        bool exists = std::filesystem::exists(path);
        std::chrono::seconds timestampSecs{timestamp};

        rv.push_back(osc::RecentFile{exists, std::move(timestampSecs), std::move(path)});
    }

    return rv;
}

// returns the filesystem path to the "recent files" file
static std::filesystem::path GetRecentFilesFilePath()
{
    return osc::GetUserDataDir() / "recent_files.txt";
}

// returns a unix timestamp in seconds since the epoch
static std::chrono::seconds GetCurrentTimeAsUnixTimestamp()
{
    return std::chrono::seconds(std::time(nullptr));
}

static osc::App::Clock::duration ConvertPerfTicksToFClockDuration(Uint64 ticks, Uint64 frequency)
{
    double dticks = static_cast<double>(ticks);
    double fq = static_cast<double>(frequency);
    float dur = static_cast<float>(dticks/fq);
    return osc::App::Clock::duration{dur};
}

static osc::App::Clock::time_point ConvertPerfCounterToFClock(Uint64 ticks, Uint64 frequency)
{
    return osc::App::Clock::time_point{ConvertPerfTicksToFClockDuration(ticks, frequency)};
}

// main application state
//
// this is what "booting the application" actually initializes
class osc::App::Impl final {
public:
    void show(std::unique_ptr<Screen> s)
    {
        log::info("showing screen %s", s->name());

        if (m_CurrentScreen)
        {
            throw std::runtime_error{"tried to call App::show when a screen is already being shown: you should use `requestTransition` instead"};
        }

        m_CurrentScreen = std::move(s);
        m_NextScreen.reset();

        // ensure retained screens are destroyed when exiting this guarded path
        //
        // this means callers can call .show multiple times on the same app
        OSC_SCOPE_GUARD({ m_CurrentScreen.reset(); });
        OSC_SCOPE_GUARD({ m_NextScreen.reset(); });

        mainLoopUnguarded();
    }

    void requestTransition(std::unique_ptr<Screen> s)
    {
        m_NextScreen = std::move(s);
    }

    bool isTransitionRequested() const
    {
        return m_NextScreen != nullptr;
    }

    void requestQuit()
    {
        m_QuitRequested = true;
    }

    glm::ivec2 idims() const
    {
        auto [w, h] = sdl::GetWindowSize(m_MainWindow);
        return glm::ivec2{w, h};
    }

    glm::vec2 dims() const
    {
        auto [w, h] = sdl::GetWindowSize(m_MainWindow);
        return glm::vec2{static_cast<float>(w), static_cast<float>(h)};
    }

    float aspectRatio() const
    {
        glm::vec2 v = dims();
        return v.x / v.y;
    }

    void setShowCursor(bool v)
    {
        SDL_ShowCursor(v ? SDL_ENABLE : SDL_DISABLE);
        SDL_SetWindowGrab(m_MainWindow, v ? SDL_FALSE : SDL_TRUE);
    }

    bool isWindowFocused() const
    {
        return SDL_GetWindowFlags(m_MainWindow) & SDL_WINDOW_INPUT_FOCUS;
    }

    void makeFullscreen()
    {
        SDL_SetWindowFullscreen(m_MainWindow, SDL_WINDOW_FULLSCREEN);
    }

    void makeWindowedFullscreen()
    {
        SDL_SetWindowFullscreen(m_MainWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }

    void makeWindowed()
    {
        SDL_SetWindowFullscreen(m_MainWindow, 0);
    }

    int getMSXAASamplesRecommended() const
    {
        return m_CurrentMSXAASamples;
    }

    void setMSXAASamplesRecommended(int s)
    {
        if (s <= 0)
        {
            throw std::runtime_error{"tried to set number of samples to <= 0"};
        }

        if (s > getMSXAASamplesMax())
        {
            throw std::runtime_error{"tried to set number of multisamples higher than supported by hardware"};
        }

        if (NumBitsSetIn(s) != 1)
        {
            throw std::runtime_error{"tried to set number of multisamples to an invalid value. Must be 1, or a multiple of 2 (1x, 2x, 4x, 8x...)"};
        }

        m_CurrentMSXAASamples = s;
    }

    int getMSXAASamplesMax() const
    {
        return m_GraphicsContext.getMaxMSXAASamples();
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

    std::future<Image> requestScreenshot()
    {
        return m_GraphicsContext.requestScreenshot();
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

    uint64_t getTicks() const
    {
        return SDL_GetPerformanceCounter();
    }

    uint64_t getTickFrequency() const
    {
        return SDL_GetPerformanceFrequency();
    }

    osc::App::Clock::time_point getCurrentTime() const
    {
        return ConvertPerfCounterToFClock(SDL_GetPerformanceCounter(), m_AppCounterFq);
    }

    osc::App::Clock::time_point getAppStartupTime() const
    {
        return m_AppStartupTime;
    }

    osc::App::Clock::time_point getFrameStartTime() const
    {
        return m_FrameStartTime;
    }

    osc::App::Clock::duration getDeltaSinceAppStartup() const
    {
        return getCurrentTime() - m_AppStartupTime;
    }

    osc::App::Clock::duration getDeltaSinceLastFrame() const
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
        m_NumFramesToPoll += 2;  // HACK: some parts of ImGui require rendering 2 frames before it shows something
        SDL_PushEvent(&e);
    }

    void clearScreen(glm::vec4 const& color)
    {
        m_GraphicsContext.clearScreen(color);
    }

    osc::App::MouseState getMouseState() const
    {
        MouseState rv;

        glm::ivec2 mouseLocal;
        Uint32 ms = SDL_GetMouseState(&mouseLocal.x, &mouseLocal.y);
        rv.LeftDown = ms & SDL_BUTTON(SDL_BUTTON_LEFT);
        rv.RightDown = ms & SDL_BUTTON(SDL_BUTTON_RIGHT);
        rv.MiddleDown = ms & SDL_BUTTON(SDL_BUTTON_MIDDLE);
        rv.X1Down = ms & SDL_BUTTON(SDL_BUTTON_X1);
        rv.X2Down = ms & SDL_BUTTON(SDL_BUTTON_X2);

        if (isWindowFocused())
        {
            static bool canUseGlobalMouseState = strncmp(SDL_GetCurrentVideoDriver(), "wayland", 7) != 0;

            if (canUseGlobalMouseState)
            {
                glm::ivec2 mouseGlobal;
                SDL_GetGlobalMouseState(&mouseGlobal.x, &mouseGlobal.y);
                glm::ivec2 mouseWindow;
                SDL_GetWindowPosition(m_MainWindow, &mouseWindow.x, &mouseWindow.y);

                rv.pos = mouseGlobal - mouseWindow;
            }
            else
            {
                rv.pos = mouseLocal;
            }
        }

        return rv;
    }

    void warpMouseInWindow(glm::vec2 v) const
    {
        SDL_WarpMouseInWindow(m_MainWindow, static_cast<int>(v.x), static_cast<int>(v.y));
    }

    bool isShiftPressed() const
    {
        return SDL_GetModState() & KMOD_SHIFT;
    }

    bool isCtrlPressed() const
    {
        return SDL_GetModState() & KMOD_CTRL;
    }

    bool isAltPressed() const
    {
        return SDL_GetModState() & KMOD_ALT;
    }

    void setMainWindowSubTitle(std::string_view sv)
    {
        // use global + mutex to prevent hopping into the OS too much
        static std::string g_CurSubtitle = "";
        static std::mutex g_SubtitleMutex;

        std::lock_guard lock{g_SubtitleMutex};

        if (sv == g_CurSubtitle)
        {
            return;
        }

        g_CurSubtitle = sv;

        std::string newTitle = sv.empty() ? g_BaseWindowTitle : (std::string{sv} + " - " + g_BaseWindowTitle);
        SDL_SetWindowTitle(m_MainWindow, newTitle.c_str());
    }

    void unsetMainWindowSubTitle()
    {
        setMainWindowSubTitle("");
    }

    osc::Config const& getConfig() const
    {
        return *m_ApplicationConfig;
    }

    Config& updConfig()
    {
        return *m_ApplicationConfig;
    }

    std::filesystem::path getResource(std::string_view p) const
    {
        return ::GetResource(*m_ApplicationConfig, p);
    }

    std::string slurpResource(std::string_view p) const
    {
        std::filesystem::path path = getResource(p);
        return SlurpFileIntoString(path);
    }

    std::vector<uint8_t> slurpBinaryResource(std::string_view p) const
    {
        std::filesystem::path path = getResource(p);
        return SlurpFileIntoVector(path);
    }

    std::vector<osc::RecentFile> getRecentFiles() const
    {
        std::filesystem::path p = GetRecentFilesFilePath();

        if (!std::filesystem::exists(p))
        {
            return {};
        }

        return LoadRecentFilesFile(p);
    }

    void addRecentFile(std::filesystem::path const& p)
    {
        std::filesystem::path recentFilesPath = GetRecentFilesFilePath();

        // load existing list
        std::vector<RecentFile> rfs;
        if (std::filesystem::exists(recentFilesPath))
        {
            rfs = LoadRecentFilesFile(recentFilesPath);
        }

        // clear potentially duplicate entries from existing list
        osc::RemoveErase(rfs, [&p](RecentFile const& rf) { return rf.path == p; });

        // write by truncating existing list file
        std::ofstream fd{recentFilesPath, std::ios::trunc};

        if (!fd)
        {
            osc::log::error("%s: could not be opened for writing: cannot update recent files list", recentFilesPath.string().c_str());
        }

        // re-serialize the n newest entries (the loaded list is sorted oldest -> newest)
        auto begin = rfs.end() - (rfs.size() < 10 ? static_cast<int>(rfs.size()) : 10);
        for (auto it = begin; it != rfs.end(); ++it)
        {
            fd << it->lastOpenedUnixTimestamp.count() << ' ' << it->path << std::endl;
        }

        // append the new entry
        fd << GetCurrentTimeAsUnixTimestamp().count() << ' ' << std::filesystem::absolute(p) << std::endl;
    }

    osc::ShaderCache& getShaderCache()
    {
        return m_ShaderCache;
    }

    osc::MeshCache& getMeshCache()
    {
        return m_MeshCache;
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

    void* updRawGLContextHandle()
    {
        return m_GraphicsContext.updRawGLContextHandle();
    }

private:
    // perform a screen transntion between two top-level `osc::Screen`s
    void transitionToNextScreen()
    {
        if (!m_NextScreen)
        {
            return;
        }

        log::info("unmounting screen %s", m_CurrentScreen->name());

        try
        {
            m_CurrentScreen->onUnmount();
        }
        catch (std::exception const& ex)
        {
            log::error("error unmounting screen %s: %s", m_CurrentScreen->name(), ex.what());
            m_CurrentScreen.reset();
            throw;
        }

        m_CurrentScreen.reset();
        m_CurrentScreen = std::move(m_NextScreen);

        // the next screen might need to draw a couple of frames
        // to "warm up" (e.g. because it's using ImGui)
        m_NumFramesToPoll = 2;

        log::info("mounting screen %s", m_CurrentScreen->name());
        m_CurrentScreen->onMount();
        log::info("transitioned main screen to %s", m_CurrentScreen->name());
    }

    // the main application loop
    //
    // this is what he application enters when it `show`s the first screen
    void mainLoopUnguarded()
    {
        // perform initial screen mount
        m_CurrentScreen->onMount();

        // ensure current screen is unmounted when exiting the main loop
        OSC_SCOPE_GUARD_IF(m_CurrentScreen, { m_CurrentScreen->onUnmount(); });

        // reset counters
        m_AppCounter = SDL_GetPerformanceCounter();
        m_FrameCounter = 0;
        m_FrameStartTime = ConvertPerfCounterToFClock(m_AppCounter, m_AppCounterFq);
        m_TimeSinceLastFrame = osc::App::Clock::duration{1.0f/60.0f};  // hack, for first frame

        while (true)  // gameloop
        {
            // pump events
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

            // update clocks
            {
                auto counter = SDL_GetPerformanceCounter();

                Uint64 deltaTicks = counter - m_AppCounter;

                m_AppCounter = counter;
                m_FrameStartTime = ConvertPerfCounterToFClock(counter, m_AppCounterFq);
                m_TimeSinceLastFrame = ConvertPerfTicksToFClockDuration(deltaTicks, m_AppCounterFq);
            }

            // "tick" the screen
            m_CurrentScreen->onTick();
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

            // "draw" the screen into the window framebuffer
            m_CurrentScreen->onDraw();

            // "present" the rendered screen to the user (can block on VSYNC)
            m_GraphicsContext.doSwapBuffers(m_MainWindow);

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

    // init/load the application config first
    std::unique_ptr<Config> m_ApplicationConfig = Config::load();

    // install the backtrace handler (if necessary - once per process)
    bool m_IsBacktraceHandlerInstalled = EnsureBacktraceHandlerEnabled();

    // init SDL context (windowing, etc.)
    sdl::Context m_SDLContext{SDL_INIT_VIDEO};

    // init main application window
    sdl::Window m_MainWindow = CreateMainAppWindow();

    // init graphics context
    GraphicsContext m_GraphicsContext{m_MainWindow};

    // get performance counter frequency (for the delta clocks)
    Uint64 m_AppCounterFq = SDL_GetPerformanceFrequency();

    // current performance counter value (recorded once per frame)
    Uint64 m_AppCounter = 0;

    // number of frames the application has drawn
    uint64_t m_FrameCounter = 0;

    // when the application started up (set now)
    App::Clock::time_point m_AppStartupTime = ConvertPerfCounterToFClock(SDL_GetPerformanceCounter(), m_AppCounterFq);

    // when the current frame started (set each frame)
    App::Clock::time_point m_FrameStartTime = m_AppStartupTime;

    // time since the frame before the current frame (set each frame)
    App::Clock::duration m_TimeSinceLastFrame = {};

    // init global shader cache
    ShaderCache m_ShaderCache{};

    // init global mesh cache
    MeshCache m_MeshCache{};

    // how many samples the implementation should actually use
    int m_CurrentMSXAASamples = std::min(m_GraphicsContext.getMaxMSXAASamples(), m_ApplicationConfig->getNumMSXAASamples());

    // set to true if the application should quit
    bool m_QuitRequested = false;

    // set to true if the main loop should pause on events
    //
    // CAREFUL: this makes the app event-driven
    bool m_InWaitMode = false;

    // set >0 to force that `n` frames are polling-driven: even in waiting mode
    int m_NumFramesToPoll = 0;

    // current screen being shown (if any)
    std::unique_ptr<Screen> m_CurrentScreen = nullptr;

    // the *next* screen the application should show
    std::unique_ptr<Screen> m_NextScreen = nullptr;
};

// public API

osc::App* osc::App::g_Current = nullptr;

osc::ShaderCache& osc::App::shaders()
{
    return upd().getShaderCache();
}

osc::MeshCache& osc::App::meshes()
{
    return upd().getMeshCache();
}

std::filesystem::path osc::App::resource(std::string_view s)
{
    return get().getResource(s);
}

std::string osc::App::slurp(std::string_view s)
{
    return get().slurpResource(s);
}

std::vector<uint8_t> osc::App::slurpBinary(std::string_view s)
{
    return get().slurpBinaryResource(std::move(s));
}

osc::App::App() : m_Impl{new Impl{}}
{
    g_Current = this;
}

osc::App::App(App&&) noexcept = default;

osc::App& osc::App::operator=(App&&) noexcept = default;

osc::App::~App() noexcept
{
    g_Current = nullptr;
}

void osc::App::show(std::unique_ptr<Screen> s)
{
    m_Impl->show(std::move(s));
}

void osc::App::requestTransition(std::unique_ptr<Screen> s)
{
    m_Impl->requestTransition(std::move(s));
}

bool osc::App::isTransitionRequested() const
{
    return m_Impl->isTransitionRequested();
}

void osc::App::requestQuit()
{
    m_Impl->requestQuit();
}

glm::ivec2 osc::App::idims() const
{
    return m_Impl->idims();
}

glm::vec2 osc::App::dims() const
{
    return m_Impl->dims();
}

float osc::App::aspectRatio() const
{
    return m_Impl->aspectRatio();
}

void osc::App::setShowCursor(bool v)
{
    m_Impl->setShowCursor(std::move(v));
}

bool osc::App::isWindowFocused() const
{
    return m_Impl->isWindowFocused();
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

int osc::App::getMSXAASamplesRecommended() const
{
    return m_Impl->getMSXAASamplesRecommended();
}

void osc::App::setMSXAASamplesRecommended(int s)
{
    m_Impl->setMSXAASamplesRecommended(std::move(s));
}

int osc::App::getMSXAASamplesMax() const
{
    return m_Impl->getMSXAASamplesMax();
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
    m_Impl->setVsync(std::move(v));
}

void osc::App::enableVsync()
{
    m_Impl->enableVsync();
}

void osc::App::disableVsync()
{
    m_Impl->disableVsync();
}

std::future<osc::Image> osc::App::requestScreenshot()
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

uint64_t osc::App::getTicks() const
{
    return m_Impl->getTicks();
}

uint64_t osc::App::getTickFrequency() const
{
    return m_Impl->getTickFrequency();
}

osc::App::Clock::time_point osc::App::getCurrentTime() const
{
    return m_Impl->getCurrentTime();
}

osc::App::Clock::time_point osc::App::getAppStartupTime() const
{
    return m_Impl->getAppStartupTime();
}

osc::App::Clock::time_point osc::App::getFrameStartTime() const
{
    return m_Impl->getFrameStartTime();
}

osc::App::Clock::duration osc::App::getDeltaSinceAppStartup() const
{
    return m_Impl->getDeltaSinceAppStartup();
}

osc::App::Clock::duration osc::App::getDeltaSinceLastFrame() const
{
    return m_Impl->getDeltaSinceLastFrame();
}

bool osc::App::isMainLoopWaiting() const
{
    return m_Impl->isMainLoopWaiting();
}

void osc::App::setMainLoopWaiting(bool v)
{
    m_Impl->setMainLoopWaiting(std::move(v));
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

void osc::App::clearScreen(glm::vec4 const& color)
{
    m_Impl->clearScreen(color);
}

osc::App::MouseState osc::App::getMouseState() const
{
    return m_Impl->getMouseState();
}

void osc::App::warpMouseInWindow(glm::vec2 v) const
{
    m_Impl->warpMouseInWindow(std::move(v));
}

bool osc::App::isShiftPressed() const
{
    return m_Impl->isShiftPressed();
}

bool osc::App::isCtrlPressed() const
{
    return m_Impl->isCtrlPressed();
}

bool osc::App::isAltPressed() const
{
    return m_Impl->isAltPressed();
}

void osc::App::setMainWindowSubTitle(std::string_view sv)
{
    m_Impl->setMainWindowSubTitle(std::move(sv));
}

void osc::App::unsetMainWindowSubTitle()
{
    m_Impl->unsetMainWindowSubTitle();
}

osc::Config const& osc::App::getConfig() const
{
    return m_Impl->getConfig();
}

osc::Config& osc::App::updConfig()
{
    return m_Impl->updConfig();
}

std::filesystem::path osc::App::getResource(std::string_view p) const
{
    return m_Impl->getResource(std::move(p));
}

std::string osc::App::slurpResource(std::string_view p) const
{
    return m_Impl->slurpResource(std::move(p));
}

std::vector<uint8_t> osc::App::slurpBinaryResource(std::string_view p) const
{
    return m_Impl->slurpBinaryResource(std::move(p));
}

std::vector<osc::RecentFile> osc::App::getRecentFiles() const
{
    return m_Impl->getRecentFiles();
}

void osc::App::addRecentFile(std::filesystem::path const& p)
{
    m_Impl->addRecentFile(p);
}

osc::ShaderCache& osc::App::getShaderCache()
{
    return m_Impl->getShaderCache();
}

osc::MeshCache& osc::App::getMeshCache()
{
    return m_Impl->getMeshCache();
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
        static std::string userIni = (osc::GetUserDataDir() / "imgui.ini").string();
        ImGui::LoadIniSettingsFromDisk(userIni.c_str());
        io.IniFilename = userIni.c_str();  // care: string has to outlive ImGui context
    }

    ImFontConfig baseConfig;
    baseConfig.SizePixels = 16.0f;
    baseConfig.PixelSnapH = true;
    baseConfig.OversampleH = 3;
    baseConfig.OversampleV = 2;
    std::string baseFontFile = App::resource("Ruda-Bold.ttf").string();
    io.Fonts->AddFontFromFileTTF(baseFontFile.c_str(), baseConfig.SizePixels, &baseConfig);

    // add FontAwesome icon support
    {
        ImFontConfig config = baseConfig;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = std::floor(1.5f * config.SizePixels);
        config.GlyphMaxAdvanceX = std::floor(1.5f * config.SizePixels);
        static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        char const* file = "fa-solid-900.ttf";
        std::string fontFile = App::resource(file).string();
        io.Fonts->AddFontFromFileTTF(fontFile.c_str(), config.SizePixels, &config, icon_ranges);
    }

    // init ImGui for SDL2 /w OpenGL
    App::Impl& impl = *App::upd().m_Impl;
    ImGui_ImplSDL2_InitForOpenGL(impl.updWindow(), impl.updRawGLContextHandle());

    // init ImGui for OpenGL
    ImGui_ImplOpenGL3_Init(OSC_GLSL_VERSION);

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
    ImGui_ImplSDL2_NewFrame(App::upd().m_Impl->updWindow());
    ImGui::NewFrame();
}

void osc::ImGuiRender()
{
    // bound program can sometimes cause issues
    App::upd().m_Impl->updGraphicsContext().clearProgram();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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

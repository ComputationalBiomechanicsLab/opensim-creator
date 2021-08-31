#include "app.hpp"

#include "osc_build_config.hpp"

#include "src/config.hpp"
#include "src/screen.hpp"
#include "src/log.hpp"
#include "src/os.hpp"
#include "src/styling.hpp"
#include "src/3d/gl.hpp"

#include "src/utils/algs.hpp"
#include "src/utils/fs.hpp"
#include "src/utils/sdl_wrapper.hpp"
#include "src/utils/scope_guard.hpp"

#include <GL/glew.h>
#include <OpenSim/Common/Logger.h>
#include <OpenSim/Actuators/RegisterTypes_osimActuators.h>
#include <OpenSim/Analyses/RegisterTypes_osimAnalyses.h>
#include <OpenSim/Common/RegisterTypes_osimCommon.h>
#include <OpenSim/Simulation/Model/ModelVisualizer.h>
#include <OpenSim/Simulation/RegisterTypes_osimSimulation.h>
#include <OpenSim/Tools/RegisterTypes_osimTools.h>
#include <OpenSim/Common/LogSink.h>
#include <OpenSim/Common/Logger.h>
#include <imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_sdl.h>

#include <fstream>


using namespace osc;

namespace {
    // install backtrace dumper
    //
    // useful if the application fails in prod: can provide some basic backtrace
    // info that users can paste into an issue or something, which is *a lot* more
    // information than "yeah, it's broke"
    bool ensure_backtrace_handler_enabled() {
        osc::log::info("enabling backtrace handler");

        static bool enabled = []() {
            install_backtrace_handler();
            return true;
        }();

        return enabled;
    }

    // returns a resource from the config-provided `resources/` dir
    std::filesystem::path get_resource(Config const& c, std::string_view p) noexcept {
        return c.resource_dir / p;
    }

    // an OpenSim log sink that sinks into OSC's main log
    class Opensim_log_sink final : public OpenSim::LogSink {
        void sinkImpl(std::string const& msg) override {
            osc::log::info("%s", msg.c_str());
        }
    };

    // initialize OpenSim for osc
    //
    // this involves setting up OpenSim's log, registering types, dirs, etc.
    bool ensure_opensim_initialized_for_osc(Config const& config) {
        static bool initialized = [&config]() {
            // disable OpenSim's `opensim.log` default
            //
            // by default, OpenSim creates an `opensim.log` file in the process's working
            // directory. This should be disabled because it screws with running multiple
            // instances of the UI on filesystems that use locking (e.g. Windows) and
            // because it's incredibly obnoxious to have `opensim.log` appear in every
            // working directory from which osc is ran
            osc::log::info("removing OpenSim's default log (opensim.log)");
            OpenSim::Logger::removeFileSink();

            // add OSC in-memory logger
            //
            // this logger collects the logs into a global mutex-protected in-memory structure
            // that the UI can can trivially render (w/o reading files etc.)
            osc::log::info("attaching OpenSim to this log");
            OpenSim::Logger::addSink(std::make_shared<Opensim_log_sink>());

            // explicitly load OpenSim libs
            //
            // this is necessary because some compilers will refuse to link a library
            // unless symbols from that library are directly used.
            //
            // Unfortunately, OpenSim relies on weak linkage *and* static library-loading
            // side-effects. This means that (e.g.) the loading of muscles into the runtime
            // happens in a static initializer *in the library*.
            //
            // osc may not link that library, though, because the source code in OSC may
            // not *directly* use a symbol exported by the library (e.g. the code might use
            // OpenSim::Muscle references, but not actually concretely refer to a muscle
            // implementation method (e.g. a ctor)
            osc::log::info("registering OpenSim types");
            RegisterTypes_osimCommon();
            RegisterTypes_osimSimulation();
            RegisterTypes_osimActuators();
            RegisterTypes_osimAnalyses();
            RegisterTypes_osimTools();

            // globally set OpenSim's geometry search path
            //
            // when an osim file contains relative geometry path (e.g. "sphere.vtp"), the
            // OpenSim implementation will look in these directories for that file
            osc::log::info("registering OpenSim geometry search path to use osc resources");
            std::filesystem::path geometry_dir = get_resource(config, "geometry");
            OpenSim::ModelVisualizer::addDirToGeometrySearchPaths(geometry_dir.string());
            osc::log::info("added geometry search path entry: %s", geometry_dir.string().c_str());

            return true;
        }();

        return initialized;
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

    // initialize the main application window
    sdl::Window create_main_app_window() {
        log::info("initializing main application (OpenGL 3.3) window");

        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        OSC_SDL_GL_SetAttribute_CHECK(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

        // careful about setting resolution, position, etc. - some people have *very* shitty
        // screens on their laptop (e.g. ultrawide, sub-HD, minus space for the start bar, can
        // be <700 px high)
        static constexpr char const* title = "OpenSim Creator v" OSC_VERSION_STRING;
        static constexpr int x = SDL_WINDOWPOS_CENTERED;
        static constexpr int y = SDL_WINDOWPOS_CENTERED;
        static constexpr int width = 800;
        static constexpr int height = 600;
        static constexpr Uint32 flags =
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;

        return sdl::CreateWindoww(title, x, y, width, height, flags);
    }

    // create an OpenGL context for an application window
    sdl::GLContext create_opengl_context(SDL_Window* window) {
        log::info("initializing application OpenGL context");

        sdl::GLContext ctx = sdl::GL_CreateContext(window);

        // enable the context
        if (SDL_GL_MakeCurrent(window, ctx) != 0) {
            throw std::runtime_error{std::string{"SDL_GL_MakeCurrent failed: "} + SDL_GetError()};
        }

        // enable vsync by default
        //
        // vsync can feel a little laggy on some systems, but vsync reduces CPU usage
        // on *constrained* systems (e.g. laptops, which the majority of users are using)
        if (SDL_GL_SetSwapInterval(-1) != 0) {
            SDL_GL_SetSwapInterval(1);
        }

        // initialize GLEW
        //
        // effectively, enables the OpenGL API used by this application
        if (auto err = glewInit(); err != GLEW_OK) {
            std::stringstream ss;
            ss << "glewInit() failed: ";
            ss << glewGetErrorString(err);
            throw std::runtime_error{ss.str()};
        }

        // depth testing used to ensure geometry overlaps correctly
        glEnable(GL_DEPTH_TEST);

        // MSXAA is used to smooth out the model
        glEnable(GL_MULTISAMPLE);

        // all vertices in the render are backface-culled
        //glEnable(GL_CULL_FACE);

        // print OpenGL information if in debug mode
        log::info(
            "OpenGL initialized: info: %s, %s, (%s), GLSL %s",
            glGetString(GL_VENDOR),
            glGetString(GL_RENDERER),
            glGetString(GL_VERSION),
            glGetString(GL_SHADING_LANGUAGE_VERSION));

        return ctx;
    }

    // returns the maximum numbers of MSXAA samples the active OpenGL context supports
    GLint get_max_msxaa_samples(sdl::GLContext const&) {
        GLint v = 1;
        glGetIntegerv(GL_MAX_SAMPLES, &v);

        // OpenGL spec: "the value must be at least 4"
        // see: https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glGet.xhtml
        if (v < 4) {
            osc::log::warn("the current OpenGl backend only supports %i samples. Technically, this is invalid (4 *should* be the minimum)", v);
        }
        OSC_ASSERT(v < 1<<16);

        return static_cast<short>(v);
    }

    // maps an OpenGL debug message severity level to a log level
    constexpr log::level::Level_enum gl_sev_to_log_lvl(GLenum sev) noexcept {
        switch (sev) {
        case GL_DEBUG_SEVERITY_HIGH: return log::level::err;
        case GL_DEBUG_SEVERITY_MEDIUM: return log::level::warn;
        case GL_DEBUG_SEVERITY_LOW: return log::level::debug;
        case GL_DEBUG_SEVERITY_NOTIFICATION: return log::level::trace;
        default: return log::level::info;
        }
    }

    // returns a string representation of an OpenGL debug message source
    constexpr char const* gl_source_to_string(GLenum src) noexcept {
        switch (src) {
        case GL_DEBUG_SOURCE_API: return "GL_DEBUG_SOURCE_API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "GL_DEBUG_SOURCE_SHADER_COMPILER";
        case GL_DEBUG_SOURCE_THIRD_PARTY: return "GL_DEBUG_SOURCE_THIRD_PARTY";
        case GL_DEBUG_SOURCE_APPLICATION: return "GL_DEBUG_SOURCE_APPLICATION";
        case GL_DEBUG_SOURCE_OTHER: return "GL_DEBUG_SOURCE_OTHER";
        default: return "GL_DEBUG_SOURCE_UNKNOWN";
        }
    }

    // returns a string representation of an OpenGL debug message type
    constexpr char const* gl_type_to_cstr(GLenum type) noexcept {
        switch (type) {
        case GL_DEBUG_TYPE_ERROR: return "GL_DEBUG_TYPE_ERROR";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
        case GL_DEBUG_TYPE_PORTABILITY: return "GL_DEBUG_TYPE_PORTABILITY";
        case GL_DEBUG_TYPE_PERFORMANCE: return "GL_DEBUG_TYPE_PERFORMANCE";
        case GL_DEBUG_TYPE_MARKER: return "GL_DEBUG_TYPE_MARKER";
        case GL_DEBUG_TYPE_PUSH_GROUP: return "GL_DEBUG_TYPE_PUSH_GROUP";
        case GL_DEBUG_TYPE_POP_GROUP: return "GL_DEBUG_TYPE_POP_GROUP";
        case GL_DEBUG_TYPE_OTHER: return "GL_DEBUG_TYPE_OTHER";
        default: return "GL_DEBUG_TYPE_UNKNOWN";
        }
    }

    // returns a string representation of an OpenGL debug message severity level
    constexpr char const* gl_sev_to_cstr(GLenum sev) noexcept {
        switch (sev) {
        case GL_DEBUG_SEVERITY_HIGH: return "GL_DEBUG_SEVERITY_HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM: return "GL_DEBUG_SEVERITY_MEDIUM";
        case GL_DEBUG_SEVERITY_LOW: return "GL_DEBUG_SEVERITY_LOW";
        case GL_DEBUG_SEVERITY_NOTIFICATION: return "GL_DEBUG_SEVERITY_NOTIFICATION";
        default: return "GL_DEBUG_SEVERITY_UNKNOWN";
        }
    }

    // returns `true` if current OpenGL context is in debug mode
    bool is_in_opengl_debug_mode() {

        // if context is not debug-mode, then some of the glGet*s below can fail
        // (e.g. GL_DEBUG_OUTPUT_SYNCHRONOUS on apple).
        {
            GLint flags;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
                return false;
            }
        }

        {
            GLboolean b = false;
            glGetBooleanv(GL_DEBUG_OUTPUT, &b);
            if (!b) {
                return false;
            }
        }

        {
            GLboolean b = false;
            glGetBooleanv(GL_DEBUG_OUTPUT_SYNCHRONOUS, &b);
            if (!b) {
                return false;
            }
        }

        return true;
    }

    // raw handler function that can be used with `glDebugMessageCallback`
    void opengl_debug_message_handler(
        GLenum source,
        GLenum type,
        unsigned int id,
        GLenum severity,
        GLsizei,
        const char* message,
        void const*) {

        log::level::Level_enum lvl = gl_sev_to_log_lvl(severity);
        char const* source_cstr = gl_source_to_string(source);
        char const* type_cstr = gl_type_to_cstr(type);
        char const* severity_cstr = gl_sev_to_cstr(severity);

        log::log(lvl,
R"(OpenGL Debug message:
    id = %u
    message = %s
    source = %s
    type = %s
    severity = %s
)", id, message, source_cstr, type_cstr, severity_cstr);
    }

    // enable OpenGL API debugging
    void enable_opengl_debug_mode() {
        if (is_in_opengl_debug_mode()) {
            log::info("application appears to already be in OpenGL debug mode: skipping enabling it");
            return;
        }

        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(opengl_debug_message_handler, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            log::info("enabled OpenGL debug mode");
        } else {
            log::error("cannot enable OpenGL debug mode: the context does not have GL_CONTEXT_FLAG_DEBUG_BIT set");
        }
    }

    // disable OpenGL API debugging
    void disable_opengl_debug_mode() {
        if (!is_in_opengl_debug_mode()) {
            log::info("application does not need to disable OpenGL debug mode: already in it: skipping");
            return;
        }

        int flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glDisable(GL_DEBUG_OUTPUT);
            log::info("disabled OpenGL debug mode");
        } else {
            log::error("cannot disable OpenGL debug mode: the context does not have a GL_CONTEXT_FLAG_DEBUG_BIT set");
        }
    }

    // returns refresh rate of highest refresh rate display on the computer
    int highest_refresh_rate_display() {
        int num_displays = SDL_GetNumVideoDisplays();

        if (num_displays < 1) {
            return 60;  // this should be impossible but, you know, coding.
        }

        int highest_refresh_rate = 30;
        SDL_DisplayMode mode_struct{};
        for (int display = 0; display < num_displays; ++display) {
            int num_modes = SDL_GetNumDisplayModes(display);
            for (int mode = 0; mode < num_modes; ++mode) {
                SDL_GetDisplayMode(display, mode, &mode_struct);
                highest_refresh_rate = std::max(highest_refresh_rate, mode_struct.refresh_rate);
            }
        }
        return highest_refresh_rate;
    }

    void imgui_apply_dark_theme_style() {
        // see: https://github.com/ocornut/imgui/issues/707
        // this one: https://github.com/ocornut/imgui/issues/707#issuecomment-512669512

        ImGui::GetStyle().FrameRounding = 4.0f;
        ImGui::GetStyle().GrabRounding = 4.0f;

        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    // load the "recent files" file that osc persists to disk
    std::vector<Recent_file> load_recent_files_file(std::filesystem::path const& p) {

        std::ifstream fd{p, std::ios::in};

        if (!fd) {
            // do not throw, because it probably shouldn't crash the application if this
            // is an issue
            osc::log::error("%s: could not be opened for reading: cannot load recent files list", p.string().c_str());
            return {};
        }

        std::vector<Recent_file> rv;
        std::string line;

        while (std::getline(fd, line)) {
            std::istringstream ss{line};

            // read line content
            uint64_t timestamp;
            std::filesystem::path path;
            ss >> timestamp;
            ss >> path;

            // calc tertiary data
            bool exists = std::filesystem::exists(path);
            std::chrono::seconds timestamp_secs{timestamp};

            rv.push_back(Recent_file{exists, std::move(timestamp_secs), std::move(path)});
        }

        return rv;
    }

    // returns a unix timestamp in seconds since the epoch
    std::chrono::seconds unix_timestamp() {
        return std::chrono::seconds(std::time(nullptr));
    }

    // returns the filesystem path to the "recent files" file
    std::filesystem::path recent_files_path() {
        return osc::user_data_dir() / "recent_files.txt";
    }
}

struct osc::App::Impl final {

    // init/load the application config first
    std::unique_ptr<Config> config = Config::load();

    // install the backtrace handler (if necessary - once per process)
    bool backtrace_handler_installed = ensure_backtrace_handler_enabled();

    // init SDL context (windowing, etc.)
    sdl::Context context{SDL_INIT_VIDEO};

    // init main application window
    sdl::Window window = create_main_app_window();

    // get performance counter frequency (for the delta clocks)
    Uint64 counter_fq = SDL_GetPerformanceFrequency();

    // init OpenGL (globally)
    sdl::GLContext gl = create_opengl_context(window);

    // figure out maximum number of samples supported by the OpenGL backend
    GLint max_msxaa_samples = get_max_msxaa_samples(gl);

    // how many samples the implementation should actually use
    GLint samples = std::min(max_msxaa_samples, config->msxaa_samples);

    // ensure OpenSim is initialized (logs, etc.)
    bool opensim_initialized = ensure_opensim_initialized_for_osc(*config);

    // set to true if the application should quit
    bool should_quit = false;

    // set to true if application is in debug mode
    bool debug_mode = false;

    // current screen being shown (if any)
    std::unique_ptr<Screen> cur_screen = nullptr;

    // the *next* screen the application should show
    //
    // this is what "requesting a transition" ultimately sets
    std::unique_ptr<Screen> next_screen = nullptr;
};

// App::Impl helper functions
namespace {

    // perform a screen transntion between two top-level `osc::Screen`s
    void do_screen_transition(App::Impl& impl) {
        impl.cur_screen->on_unmount();
        impl.cur_screen.reset();
        impl.cur_screen = std::move(impl.next_screen);
        Screen& sref = *impl.cur_screen;
        log::info("mounting screen %s", typeid(sref).name());
        impl.cur_screen->on_mount();
        log::info("transitioned main screen to %s", typeid(sref).name());
    }

    void show_UNGUARDED(App::Impl& impl) {

        // perform initial screen mount
        impl.cur_screen->on_mount();

        // ensure on_unmount is called before potentially destructing the screen
        OSC_SCOPE_GUARD_IF(impl.cur_screen, { impl.cur_screen->on_unmount(); });

        Uint64 prev_counter = 0;

        while (true) {  // gameloop

            for (SDL_Event e; SDL_PollEvent(&e);) {  // event pump

                // SDL_QUIT should cause the application to immediately quit
                if (e.type == SDL_QUIT) {
                    return;
                }

                // let screen handle the event
                impl.cur_screen->on_event(e);

                // event handling may have requested a quit
                if (impl.should_quit) {
                    return;
                }

                // event handling may have requested a screen transition
                if (impl.next_screen) {
                    do_screen_transition(impl);
                }
            }

            // figure out frame delta
            Uint64 counter = SDL_GetPerformanceCounter();
            float dt;
            if (prev_counter > 0) {
                Uint64 ticks = counter - prev_counter;
                dt = static_cast<float>(static_cast<double>(ticks) / impl.counter_fq);
            } else {
                dt = 1.0f/60.0f;  // first iteration
            }
            prev_counter = counter;

            // "tick" the screen
            impl.cur_screen->tick(dt);

            // "tick" may have requested a quit
            if (impl.should_quit) {
                return;
            }

            // "tick" may have requested a screen transition
            if (impl.next_screen) {
                do_screen_transition(impl);
                continue;
            }

            // "draw" the screen into the window framebuffer
            impl.cur_screen->draw();

            // "present" the rendered screen to the user (can block on VSYNC)
            SDL_GL_SwapWindow(impl.window);

            // "draw" may have requested a quit
            if (impl.should_quit) {
                return;
            }

            // "draw" may have requested a transition
            if (impl.next_screen) {
                do_screen_transition(impl);
                continue;
            }
        }
    }

    void show(App::Impl& impl, std::unique_ptr<Screen> s) {
        {
            Screen& sref = *s;
            log::info("starting application main render loop with screen %s", typeid(sref).name());
        }

        if (impl.cur_screen) {
            throw std::runtime_error{"tried to call App::show when a screen is already being shown: you should use `request_transition` instead"};
        }

        impl.cur_screen = std::move(s);
        impl.next_screen.reset();

        // ensure screens are cleaned up - regardless of how `show` is exited from
        OSC_SCOPE_GUARD({ impl.cur_screen.reset(); impl.next_screen.reset(); });

        try {
            show_UNGUARDED(impl);
        } catch (std::exception const& ex) {
            log::error("unhandled exception thrown in main render loop: %s", ex.what());
            throw;
        }
    }

    void enable_debug_mode(App::Impl& impl) {
        if (is_in_opengl_debug_mode()) {
            return;  // already in debug mode
        }

        log::info("enabling debug mode");
        enable_opengl_debug_mode();
        impl.debug_mode = true;
    }

    void disable_debug_mode(App::Impl& impl) {
        if (!is_in_opengl_debug_mode()) {
            return;  // already not in debug mode
        }

        log::info("disabling debug mode");
        disable_opengl_debug_mode();
        impl.debug_mode = false;
    }
}


// public API

osc::App* osc::App::g_Current = nullptr;

osc::App::App() : impl{new Impl{}} {
    g_Current = this;
}

osc::App::App(App&&) noexcept = default;

osc::App::~App() noexcept {
    g_Current = nullptr;
}

osc::App& osc::App::operator=(App&&) noexcept = default;

void osc::App::show(std::unique_ptr<Screen> s) {
    ::show(*impl, std::move(s));
}

void osc::App::request_transition(std::unique_ptr<Screen> s) {
    impl->next_screen = std::move(s);
}

void osc::App::request_quit() {
    impl->should_quit = true;
}

glm::ivec2 osc::App::idims() const noexcept {
    auto [w, h] = sdl::GetWindowSize(impl->window);
    return glm::ivec2{w, h};
}

glm::vec2 osc::App::dims() const noexcept {
    auto [w, h] = sdl::GetWindowSize(impl->window);
    return glm::vec2{static_cast<float>(w), static_cast<float>(h)};
}

float osc::App::aspect_ratio() const noexcept {
    glm::vec2 v = dims();
    return v.x / v.y;
}

void osc::App::set_relative_mouse_mode() noexcept {
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void osc::App::make_fullscreen() {
    SDL_SetWindowFullscreen(impl->window, SDL_WINDOW_FULLSCREEN);
}

void osc::App::make_windowed() {
    SDL_SetWindowFullscreen(impl->window, 0);
}

int osc::App::get_samples() const noexcept {
    return impl->samples;
}

void osc::App::set_samples(int s) {
    if (s <= 0) {
        throw std::runtime_error{"tried to set number of samples to <= 0"};
    }

    if (s > max_samples()) {
        throw std::runtime_error{"tried to set number of multisamples higher than supported by hardware"};
    }

    if (num_bits_set_in(s) != 1) {
        throw std::runtime_error{
            "tried to set number of multisamples to an invalid value. Must be 1, or a multiple of 2 (1x, 2x, 4x, 8x...)"};
    }

    impl->samples = s;
}

int osc::App::max_samples() const noexcept {
    return impl->max_msxaa_samples;
}

bool osc::App::is_in_debug_mode() const noexcept {
    return impl->debug_mode;
}

void osc::App::enable_debug_mode() {
    ::enable_debug_mode(*impl);
}

void osc::App::disable_debug_mode() {
    ::disable_debug_mode(*impl);
}

bool osc::App::is_vsync_enabled() const noexcept {
    // adaptive vsync (-1) and vsync (1) are treated as "vsync is enabled"
    return SDL_GL_GetSwapInterval() != 0;
}

void osc::App::enable_vsync() {
    // try using adaptive vsync
    if (SDL_GL_SetSwapInterval(-1) == 0) {
        return;
    }

    // if adaptive vsync doesn't work, then try normal vsync
    if (SDL_GL_SetSwapInterval(1) == 0) {
        return;
    }

    // otherwise, setting vsync isn't supported by the system
}

void osc::App::disable_vsync() {
    SDL_GL_SetSwapInterval(0);
}

Config const& osc::App::get_config() const noexcept {
    return *impl->config;
}

std::filesystem::path osc::App::get_resource(std::string_view p) const noexcept {
    return ::get_resource(*impl->config, p);
}

std::string osc::App::slurp_resource(std::string_view p) const {
    std::filesystem::path path = get_resource(p);
    return slurp_into_string(path);
}

std::vector<Recent_file> osc::App::recent_files() const {
    std::filesystem::path p = recent_files_path();

    if (!std::filesystem::exists(p)) {
        return {};
    }

    return load_recent_files_file(p);
}

void osc::App::add_recent_file(std::filesystem::path const& p) {
    std::filesystem::path rfs_path = recent_files_path();

    // load existing list
    std::vector<Recent_file> rfs;
    if (std::filesystem::exists(rfs_path)) {
        rfs = load_recent_files_file(rfs_path);
    }

    // clear potentially duplicate entries from existing list
    osc::remove_erase(rfs, [&p](Recent_file const& rf) { return rf.path == p; });

    // write by truncating existing list file
    std::ofstream fd{rfs_path, std::ios::trunc};

    if (!fd) {
        osc::log::error("%s: could not be opened for writing: cannot update recent files list", rfs_path.string().c_str());
    }

    // re-serialize the n newest entries (the loaded list is sorted oldest -> newest)
    auto begin = rfs.end() - (rfs.size() < 10 ? static_cast<int>(rfs.size()) : 10);
    for (auto it = begin; it != rfs.end(); ++it) {
        fd << it->unix_timestamp.count() << ' ' << it->path << std::endl;
    }

    // append the new entry
    fd << unix_timestamp().count() << ' ' << std::filesystem::absolute(p) << std::endl;
}

void osc::ImGuiInit() {

    // init ImGui top-level context
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    // configure ImGui from OSC's (toml) configuration
    {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        if (App::config().use_multi_viewport) {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        }
    }

    // load application-level ImGui config, then the user one,
    // so that the user config takes precedence
    {
        std::string default_ini = App::resource("imgui_base_config.ini").string();
        ImGui::LoadIniSettingsFromDisk(default_ini.c_str());
        static std::string user_inifile = (osc::user_data_dir() / "imgui.ini").string();
        ImGui::LoadIniSettingsFromDisk(user_inifile.c_str());
        io.IniFilename = user_inifile.c_str();
    }

    // add FontAwesome icon support
    {
        io.Fonts->AddFontDefault();
        ImFontConfig config;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = 13.0f;  // monospaced
        static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        std::string fa_font = App::resource("fontawesome-webfont.ttf").string();
        ImGui::GetIO().Fonts->AddFontFromFileTTF(fa_font.c_str(), 13.0f, &config, icon_ranges);
    }

    // init ImGui for SDL2 /w OpenGL
    ImGui_ImplSDL2_InitForOpenGL(App::cur().impl->window, App::cur().impl->gl);

    // init ImGui for OpenGL
    ImGui_ImplOpenGL3_Init(OSC_GLSL_VERSION);

    imgui_apply_dark_theme_style();
}

void osc::ImGuiShutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool osc::ImGuiOnEvent(SDL_Event const& e) {
    ImGui_ImplSDL2_ProcessEvent(&e);

    ImGuiIO const& io  = ImGui::GetIO();

    if (io.WantCaptureKeyboard && (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)) {
        return true;
    }

    if (io.WantCaptureMouse && (e.type == SDL_MOUSEWHEEL || e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONUP || e.type == SDL_MOUSEBUTTONDOWN)) {
        return true;
    }

    return false;
}

void osc::ImGuiNewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(App::cur().impl->window);
    ImGui::NewFrame();
}

void osc::ImGuiRender() {
    gl::UseProgram();  // bound program can sometimes cause issues

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // ImGui: handle multi-viewports if the user has requested them
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
    }
}

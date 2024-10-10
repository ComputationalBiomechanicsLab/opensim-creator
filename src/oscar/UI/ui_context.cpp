#include "ui_context.h"

#include <oscar/Maths/RectFunctions.h>
#include <oscar/Platform/App.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/Platform/Cursor.h>
#include <oscar/Platform/CursorShape.h>
#include <oscar/Platform/Event.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/os.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Shims/Cpp20/bit.h>
#include <oscar/Shims/Cpp23/ranges.h>
#include <oscar/UI/ImGuizmo.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/ui_graphics_backend.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/Conversion.h>
#include <oscar/Utils/Perf.h>

#define IMGUI_USER_CONFIG <oscar/UI/oscimgui_config.h>  // NOLINT(bugprone-macro-parentheses)
#include <imgui.h>
#include <implot.h>
#include <SDL.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/em_js.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>

#if SDL_VERSION_ATLEAST(2,0,4) && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && !(defined(__APPLE__) && TARGET_OS_IOS) && !defined(__amigaos4__)
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    1  // NOLINT(cppcoreguidelines-macro-usage)
#else
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    0  // NOLINT(cppcoreguidelines-macro-usage)
#endif

using namespace osc;
namespace rgs = std::ranges;

template<>
struct osc::Converter<ImGuiMouseCursor, CursorShape> final {
    CursorShape operator()(ImGuiMouseCursor cursor) const
    {
        static_assert(ImGuiMouseCursor_COUNT == 9);

        switch (cursor) {
        case ImGuiMouseCursor_None:       return CursorShape::Hidden;
        case ImGuiMouseCursor_Arrow:      return CursorShape::Arrow;
        case ImGuiMouseCursor_TextInput:  return CursorShape::IBeam;
        case ImGuiMouseCursor_ResizeAll:  return CursorShape::ResizeAll;
        case ImGuiMouseCursor_ResizeNS:   return CursorShape::ResizeVertical;
        case ImGuiMouseCursor_ResizeEW:   return CursorShape::ResizeHorizontal;
        case ImGuiMouseCursor_ResizeNESW: return CursorShape::ResizeDiagonalNESW;
        case ImGuiMouseCursor_ResizeNWSE: return CursorShape::ResizeDiagonalNWSE;
        case ImGuiMouseCursor_Hand:       return CursorShape::PointingHand;
        case ImGuiMouseCursor_NotAllowed: return CursorShape::Forbidden;
        default:                          return CursorShape::Arrow;
        }
    }
};

namespace
{
    // Returns whether global (OS-level, rather than window-level) mouse data
    // can be acquired from the OS.
    bool can_mouse_use_global_state()
    {
        // Check and store if we are on a SDL backend that supports global mouse position
        // ("wayland" and "rpi" don't support it, but we chose to use a white-list instead of a black-list)
        bool mouse_can_use_global_state = false;
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
        const auto sdl_backend = std::string_view{SDL_GetCurrentVideoDriver()};
        const auto global_mouse_whitelist = std::to_array<std::string_view>({"windows", "cocoa", "x11", "DIVE", "VMAN"});
        mouse_can_use_global_state = rgs::any_of(global_mouse_whitelist, [sdl_backend](std::string_view whitelisted) { return sdl_backend.starts_with(whitelisted); });
#endif
        return mouse_can_use_global_state;
    }

    // Returns whether the global hover state of the mouse can be queried to ask if it's
    // currently hovering a given UI viewport.
    bool can_mouse_report_hovered_viewport(bool mouse_can_use_global_state)
    {
        // SDL on Linux/OSX doesn't report events for unfocused windows (see https://github.com/ocornut/imgui/issues/4960)
        // We will use 'MouseCanReportHoveredViewport' to set 'ImGuiBackendFlags_HasMouseHoveredViewport' dynamically each frame.
#ifndef __APPLE__
        return mouse_can_use_global_state;
#else
        return false;
#endif
    }

    // The internal backend data associated with one UI context.
    struct BackendData final {

        explicit BackendData(SDL_Window* window) :
            Window{window}
        {}

        SDL_Window*                                      Window = nullptr;
        std::string                                      ClipboardText;
        bool                                             WantUpdateMonitors = true;

        // Mouse handling
        Uint32                                           MouseWindowID = 0;
        std::optional<CursorShape>                       CurrentCustomCursor;
        int                                              MouseButtonsDown = 0;
        int                                              MouseLastLeaveFrame = 0;
        bool                                             MouseCanUseGlobalState = can_mouse_use_global_state();
        // This is hard to use/unreliable on SDL so we'll set ImGuiBackendFlags_HasMouseHoveredViewport dynamically based on state.
        bool                                             MouseCanReportHoveredViewport = can_mouse_report_hovered_viewport(MouseCanUseGlobalState);
    };

    // Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
    // It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
    // FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
    // FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
    BackendData* try_get_ui_backend_data()
    {
        return ImGui::GetCurrentContext() ? static_cast<BackendData*>(ImGui::GetIO().BackendPlatformUserData) : nullptr;
    }

    BackendData& get_backend_data()
    {
        BackendData* bd = try_get_ui_backend_data();
        IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplOscar_Init()?");
        return *bd;
    }

    ImGuiPlatformMonitor to_ith_ui_monitor(const Monitor& os_monitor, size_t i)
    {
        ImGuiPlatformMonitor rv;
        rv.MainPos = os_monitor.bounds().p1;
        rv.MainSize = dimensions_of(os_monitor.bounds());
        rv.WorkPos = os_monitor.usable_bounds().p1;
        rv.WorkSize = dimensions_of(os_monitor.usable_bounds());
        rv.DpiScale = os_monitor.physical_dpi() / 96.0f;
        rv.PlatformHandle = cpp20::bit_cast<void*>(i);
        return rv;
    }

    void update_monitors(const App& app)
    {
        const auto os_monitors = app.monitors();
        auto& ui_monitors = ImGui::GetPlatformIO().Monitors;

        ui_monitors.clear();
        ui_monitors.reserve(static_cast<int>(os_monitors.size()));
        for (size_t i = 0; i < os_monitors.size(); ++i) {
            ui_monitors.push_back(to_ith_ui_monitor(os_monitors[i], i));
        }
    }

    const char* ui_get_clipboard_text(void*)
    {
        BackendData* bd = try_get_ui_backend_data();
        bd->ClipboardText = get_clipboard_text();
        return bd->ClipboardText.c_str();
    }

    void ui_set_clipboard_text(void*, const char* text)
    {
        set_clipboard_text(text);
    }
}

// Note: native IME will only display if user calls SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1") _before_ SDL_CreateWindow().
static void ImGui_ImplOscar_PlatformSetImeData(ImGuiContext*, ImGuiViewport*, ImGuiPlatformImeData* data)
{
    if (data->WantVisible) {
        const Vec2 top_left = {data->InputPos.x, data->InputPos.y};
        const Vec2 dimensions = {1.0f, data->InputLineHeight};
        App::upd().set_unicode_input_rect(Rect{top_left, top_left + dimensions});
    }
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
// If you have multiple SDL events and some of them are not meant to be used by dear imgui, you may need to filter events based on their windowID field.
static bool ImGui_ImplOscar_ProcessEvent(Event& e)
{
    ImGuiIO& io = ImGui::GetIO();
    BackendData* bd = try_get_ui_backend_data();

    switch (e.type()) {
    case EventType::MouseMove: {
        const auto& move_event = dynamic_cast<const MouseEvent&>(e);
        io.AddMouseSourceEvent(move_event.input_source() == MouseInputSource::TouchScreen ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
        io.AddMousePosEvent(move_event.position_in_window().x, move_event.position_in_window().y);
        return true;
    }
    case EventType::MouseWheel: {
        const auto& wheel_event = dynamic_cast<const MouseWheelEvent&>(e);
        auto [x, y] = wheel_event.delta();
#ifdef __EMSCRIPTEN__
        x /= 100.0f;
#endif
        io.AddMouseSourceEvent(wheel_event.input_source() == MouseInputSource::TouchScreen ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
        io.AddMouseWheelEvent(x, y);
        return true;
    }
    case EventType::MouseButtonDown:
    case EventType::MouseButtonUp: {
        const auto& button_event = dynamic_cast<const MouseEvent&>(e);
        const auto button = button_event.button();

        int mouse_button = -1;
        if (button == MouseButton::Left)    { mouse_button = 0; }
        if (button == MouseButton::Right)   { mouse_button = 1; }
        if (button == MouseButton::Middle)  { mouse_button = 2; }
        if (button == MouseButton::Back)    { mouse_button = 3; }
        if (button == MouseButton::Forward) { mouse_button = 4; }

        if (mouse_button == -1) {
            return false;
        }

        io.AddMouseSourceEvent(button_event.input_source() == MouseInputSource::TouchScreen ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
        io.AddMouseButtonEvent(mouse_button, button_event.type() == EventType::MouseButtonDown);
        bd->MouseButtonsDown = (button_event.type() == EventType::MouseButtonDown) ? (bd->MouseButtonsDown | (1 << mouse_button)) : (bd->MouseButtonsDown & ~(1 << mouse_button));
        return true;
    }
    case EventType::KeyDown:
    case EventType::KeyUp: {
        const auto& key_event = dynamic_cast<const KeyEvent&>(e);
        io.AddKeyEvent(ImGuiMod_Ctrl, key_event.modifier() & KeyModifier::Ctrl);
        io.AddKeyEvent(ImGuiMod_Shift, key_event.modifier() & KeyModifier::Shift);
        io.AddKeyEvent(ImGuiMod_Alt, key_event.modifier() & KeyModifier::Alt);
        io.AddKeyEvent(ImGuiMod_Super, key_event.modifier() & KeyModifier::Gui);
        io.AddKeyEvent(to<ImGuiKey>(key_event.key()), key_event.type() == EventType::KeyDown);
        return true;
    }
    case EventType::TextInput: {
        const auto& text_event = dynamic_cast<const TextInputEvent&>(e);
        io.AddInputCharactersUTF8(text_event.utf8_text().c_str());
        return true;
    }
    case EventType::DisplayStateChange: {
        // 2.0.26 has SDL_DISPLAYEVENT_CONNECTED/SDL_DISPLAYEVENT_DISCONNECTED/SDL_DISPLAYEVENT_ORIENTATION,
        // so change of DPI/Scaling are not reflected in this event. (SDL3 has it)
        bd->WantUpdateMonitors = true;
        return true;
    }
    case EventType::Window: {
        // - When capturing mouse, SDL will send a bunch of conflicting LEAVE/ENTER event on every mouse move, but the final ENTER tends to be right.
        // - However we won't get a correct LEAVE event for a captured window.
        // - In some cases, when detaching a window from main viewport SDL may send SDL_WINDOWEVENT_ENTER one frame too late,
        //   causing SDL_WINDOWEVENT_LEAVE on previous frame to interrupt drag operation by clear mouse position. This is why
        //   we delay process the SDL_WINDOWEVENT_LEAVE events by one frame. See issue #5012 for details.
        const auto& window_event = dynamic_cast<const WindowEvent&>(e);

        switch (window_event.type()) {
        case WindowEventType::GainedMouseFocus: {
            bd->MouseWindowID = window_event.window_id();
            bd->MouseLastLeaveFrame = 0;
            return true;
        }
        case WindowEventType::LostMouseFocus: {
            bd->MouseLastLeaveFrame = ImGui::GetFrameCount() + 1;
            return true;
        }
        case WindowEventType::GainedKeyboardFocus: {
            io.AddFocusEvent(true);
            return true;
        }
        case WindowEventType::LostKeyboardFocus: {
            io.AddFocusEvent(false);
            return true;
        }
        case WindowEventType::WindowClosed: {
            if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(cpp20::bit_cast<void*>(window_event.window()))) {
                viewport->PlatformRequestClose = true;
            }
            return true;
        }
        case WindowEventType::WindowMoved: {
            if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(cpp20::bit_cast<void*>(window_event.window()))) {
                viewport->PlatformRequestMove = true;
            }
            return true;
        }
        case WindowEventType::WindowResized: {
            if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(cpp20::bit_cast<void*>(window_event.window()))) {
                viewport->PlatformRequestResize = true;
            }
            return true;
        }
        default: {
            return true;
        }
        }
    }
    default: {
        return false;
    }
    }
}

#ifdef __EMSCRIPTEN__
EM_JS(void, ImGui_ImplOscar_EmscriptenOpenURL, (char const* url), { url = url ? UTF8ToString(url) : null; if (url) window.open(url, '_blank'); });
#endif

static void ImGui_ImplOscar_Init(SDL_Window* window)
{
    ImGuiIO& io = ImGui::GetIO();
    OSC_ASSERT_ALWAYS(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    // init `BackendData` and setup `ImGuiIO` pointers etc.
    io.BackendPlatformUserData = static_cast<void*>(new BackendData{window});
    io.BackendPlatformName = "imgui_impl_oscar";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;            // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.SetClipboardTextFn = ui_set_clipboard_text;
    io.GetClipboardTextFn = ui_get_clipboard_text;
    io.ClipboardUserData = nullptr;
    io.PlatformSetImeDataFn = ImGui_ImplOscar_PlatformSetImeData;
#ifdef __EMSCRIPTEN__
    io.PlatformOpenInShellFn = [](ImGuiContext*, const char* url) { ImGui_ImplOscar_EmscriptenOpenURL(url); return true; };
#endif

    // init `ImGuiViewport` for main viewport
    //
    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = cpp20::bit_cast<void*>(window);
    main_viewport->PlatformHandleRaw = nullptr;
}

static void ImGui_ImplOscar_Shutdown(App& app)
{
    BackendData* bd = try_get_ui_backend_data();
    OSC_ASSERT_ALWAYS(bd != nullptr && "No platform backend to shutdown, or already shutdown?");

    if (bd->CurrentCustomCursor) {
        app.pop_cursor_override();
    }

    delete bd;  // NOLINT(cppcoreguidelines-owning-memory)

    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_HasMouseHoveredViewport);
}

// This code is incredibly messy because some of the functions we need for full viewport support are not available in SDL < 2.0.4.
static void ImGui_ImplSDL2_UpdateMouseData()
{
    BackendData* bd = try_get_ui_backend_data();
    ImGuiIO& io = ImGui::GetIO();

    // We forward mouse input when hovered or captured (via SDL_MOUSEMOTION) or when focused (below)
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    // SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the SDL window boundaries shouldn't e.g. trigger other operations outside
    SDL_CaptureMouse((bd->MouseButtonsDown != 0) ? SDL_TRUE : SDL_FALSE);
    SDL_Window* focused_window = SDL_GetKeyboardFocus();
    const bool is_app_focused = (focused_window != nullptr && (bd->Window == focused_window || ImGui::FindViewportByPlatformHandle(cpp20::bit_cast<void*>(focused_window)) != nullptr));
#else
    SDL_Window* focused_window = bd->Window;
    const bool is_app_focused = (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_INPUT_FOCUS) != 0; // SDL 2.0.3 and non-windowed systems: single-viewport only
#endif

    if (is_app_focused) {
        // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if (io.WantSetMousePos) {
            if (SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE and (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)) {
                SDL_WarpMouseGlobal(static_cast<int>(io.MousePos.x), static_cast<int>(io.MousePos.y));
            }
            else {
                SDL_WarpMouseInWindow(bd->Window, static_cast<int>(io.MousePos.x), static_cast<int>(io.MousePos.y));
            }
        }

        // (Optional) Fallback to provide mouse position when focused (SDL_MOUSEMOTION already provides this when hovered or captured)
        if (bd->MouseCanUseGlobalState && bd->MouseButtonsDown == 0) {
            // Single-viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
            // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
            int mouse_x = 0;
            int mouse_y = 0;
            SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
            if (!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)) {
                int window_x = 0;
                int window_y = 0;
                SDL_GetWindowPosition(focused_window, &window_x, &window_y);
                mouse_x -= window_x;
                mouse_y -= window_y;
            }
            io.AddMousePosEvent(static_cast<float>(mouse_x), static_cast<float>(mouse_y));
        }
    }

    // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the viewport the OS mouse cursor is hovering.
    // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear imGui will ignore this field and infer the information using its flawed heuristic.
    // - [!] SDL backend does NOT correctly ignore viewports with the _NoInputs flag.
    //       Some backend are not able to handle that correctly. If a backend report an hovered viewport that has the _NoInputs flag (e.g. when dragging a window
    //       for docking, the viewport has the _NoInputs flag in order to allow us to find the viewport under), then Dear ImGui is forced to ignore the value reported
    //       by the backend, and use its flawed heuristic to guess the viewport behind.
    // - [X] SDL backend correctly reports this regardless of another viewport behind focused and dragged from (we need this to find a useful drag and drop target).
    if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport) {
        ImGuiID mouse_viewport_id = 0;
        if (SDL_Window* sdl_mouse_window = SDL_GetWindowFromID(bd->MouseWindowID)) {
            if (ImGuiViewport* mouse_viewport = ImGui::FindViewportByPlatformHandle(cpp20::bit_cast<void*>(sdl_mouse_window))) {
                mouse_viewport_id = mouse_viewport->ID;
            }
        }
        io.AddMouseViewportEvent(mouse_viewport_id);
    }
}

static void ImGui_ImplOscar_UpdateMouseCursor(App& app)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
        return;  // ui cannot change the mouse cursor
    }

    BackendData& bd = get_backend_data();
    const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    const CursorShape oscar_cursor = to<CursorShape>(imgui_cursor);

    if (oscar_cursor != bd.CurrentCustomCursor) {
        if (bd.CurrentCustomCursor) {
            app.pop_cursor_override();
        }
        app.push_cursor_override(Cursor{oscar_cursor});
        bd.CurrentCustomCursor = oscar_cursor;
    }
}

static void ImGui_ImplOscar_NewFrame(App& app)
{
    BackendData& bd = get_backend_data();
    ImGuiIO& io = ImGui::GetIO();

    // Setup `DisplaySize` and `DisplayFramebufferScale`
    //
    // Performed every frame to accomodate for runtime window resizes
    {
        auto window_dimensions = app.main_window_dimensions();
        if (app.is_main_window_minimized()) {
            window_dimensions = {0.0f, 0.0f};
        }
        io.DisplaySize = {window_dimensions.x, window_dimensions.y};

        const auto window_drawable_dimensions = app.main_window_drawable_pixel_dimensions();
        if (area_of(window_dimensions) > 0.0f) {
            io.DisplayFramebufferScale = window_drawable_dimensions / window_dimensions;
        }
    }

    // Update monitors
    if (bd.WantUpdateMonitors) {
        update_monitors(app);
        bd.WantUpdateMonitors = false;
    }

    // Update `DeltaTime`
    io.DeltaTime = static_cast<float>(app.frame_delta_since_last_frame().count());

    // Handle mouse leaving the window
    if (bd.MouseLastLeaveFrame and (bd.MouseLastLeaveFrame >= ImGui::GetFrameCount()) and bd.MouseButtonsDown == 0) {
        bd.MouseWindowID = 0;
        bd.MouseLastLeaveFrame = 0;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    // Our io.AddMouseViewportEvent() calls will only be valid when not capturing.
    // Technically speaking testing for 'bd->MouseButtonsDown == 0' would be more rygorous, but testing for payload reduces noise and potential side-effects.
    if (bd.MouseCanReportHoveredViewport and ImGui::GetDragDropPayload() == nullptr) {
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
    }
    else {
        io.BackendFlags &= ~ImGuiBackendFlags_HasMouseHoveredViewport;
    }

    ImGui_ImplSDL2_UpdateMouseData();
    ImGui_ImplOscar_UpdateMouseCursor(app);
}

namespace
{
#ifndef EMSCRIPTEN
    // this is necessary because ImGui will take ownership and be responsible for
    // freeing the memory with `ImGui::MemFree`
    char* to_imgui_allocated_copy(std::span<const char> span)
    {
        auto* ptr = cpp20::bit_cast<char*>(ImGui::MemAlloc(span.size_bytes()));
        rgs::copy(span, ptr);
        return ptr;
    }

    void add_resource_as_font(
        ResourceLoader& loader,
        const ImFontConfig& config,
        ImFontAtlas& atlas,
        const ResourcePath& path,
        const ImWchar* glyph_ranges = nullptr)
    {
        const std::string base_font_data = loader.slurp(path);
        const std::span<const char> data_including_nul_terminator{base_font_data.data(), base_font_data.size() + 1};

        atlas.AddFontFromMemoryTTF(
            to_imgui_allocated_copy(data_including_nul_terminator),
            static_cast<int>(data_including_nul_terminator.size()),
            config.SizePixels,
            &config,
            glyph_ranges
        );
    }
#endif
}

void osc::ui::context::init(App& app)
{
    // ensure ImGui uses the same allocator as the rest of
    // our (C++ stdlib) application
    ImGui::SetAllocatorFunctions(
        [](size_t count, [[maybe_unused]] void* user_data) { return ::operator new(count); },
        [](void* ptr, [[maybe_unused]] void* user_data) { ::operator delete(ptr); }
    );

    // init ImGui top-level context
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // make it so that windows can only ever be moved from the title bar
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // load application-level ImGui settings, then the user one,
    // so that the user settings takes precedence
#ifdef EMSCRIPTEN
    io.IniFilename = nullptr;
#else
    float dpi_scale_factor = [&]()
    {
        // if the user explicitly enabled high_dpi_mode...
        if (auto v = app.get_config().find_value("experimental_feature_flags/high_dpi_mode"); v and *v) {
            return app.main_window_dpi() / 96.0f;
        }
        else {
            return 1.0f;  // else: assume it's an unscaled 96dpi screen
        }
    }();

    {
        const std::string base_ini_data = app.slurp_resource("imgui_base_config.ini");
        ImGui::LoadIniSettingsFromMemory(base_ini_data.data(), base_ini_data.size());

        // CARE: the reason this filepath is `static` is because ImGui requires that
        // the string outlives the ImGui context
        static const std::string s_user_imgui_ini_file_path = (app.user_data_directory() / "imgui.ini").string();

        ImGui::LoadIniSettingsFromDisk(s_user_imgui_ini_file_path.c_str());
        io.IniFilename = s_user_imgui_ini_file_path.c_str();
    }

    ImFontConfig base_config;
    base_config.SizePixels = dpi_scale_factor*15.0f;
    base_config.PixelSnapH = true;
    base_config.FontDataOwnedByAtlas = true;
    add_resource_as_font(app.upd_resource_loader(), base_config, *io.Fonts, "oscar/fonts/Ruda-Bold.ttf");

    // add FontAwesome icon support
    {
        ImFontConfig config = base_config;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = floor(1.5f * config.SizePixels);
        config.GlyphMaxAdvanceX = floor(1.5f * config.SizePixels);
        static constexpr auto c_icon_ranges = std::to_array<ImWchar>({ OSC_ICON_MIN, OSC_ICON_MAX, 0 });
        add_resource_as_font(app.upd_resource_loader(), config, *io.Fonts, "oscar/fonts/fa-solid-900.ttf", c_icon_ranges.data());
    }
#endif

    // init ImGui for oscar
    ImGui_ImplOscar_Init(app.upd_underlying_window());

    // init ImGui for oscar's graphics backend (OpenGL)
    graphics_backend::init();

    apply_dark_theme();

    // init extra parts (plotting, gizmos, etc.)
    ImPlot::CreateContext();
    ImGuizmo::CreateContext();
}

void osc::ui::context::shutdown(App& app)
{
    ImGuizmo::DestroyContext();
    ImPlot::DestroyContext();

    graphics_backend::shutdown();
    ImGui_ImplOscar_Shutdown(app);
    ImGui::DestroyContext();
}

bool osc::ui::context::on_event(Event& ev)
{
    ImGui_ImplOscar_ProcessEvent(ev);

    // handle `.WantCaptureKeyboard`
    constexpr auto keyboard_event_types = std::to_array({EventType::KeyDown, EventType::KeyUp});
    if (ImGui::GetIO().WantCaptureKeyboard and cpp23::contains(keyboard_event_types, ev.type())) {
        return true;
    }

    // handle `.WantCaptureMouse`
    constexpr auto mouse_event_types = std::to_array({EventType::MouseWheel, EventType::MouseMove, EventType::MouseButtonUp, EventType::MouseButtonDown});
    if (ImGui::GetIO().WantCaptureMouse and cpp23::contains(mouse_event_types, ev.type())) {
        return true;
    }

    return false;
}

void osc::ui::context::on_start_new_frame(App& app)
{
    graphics_backend::on_start_new_frame();
    ImGui_ImplOscar_NewFrame(app);
    ImGui::NewFrame();

    // extra parts
    ImGuizmo::BeginFrame();
}

void osc::ui::context::render()
{
    {
        OSC_PERF("ImGui::Render()");
        ImGui::Render();
    }

    {
        OSC_PERF("graphics_backend::render(ImGui::GetDrawData())");
        graphics_backend::render(ImGui::GetDrawData());
    }
}

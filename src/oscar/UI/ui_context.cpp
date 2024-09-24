#include "ui_context.h"

#include <oscar/Platform/App.h>
#include <oscar/Platform/AppSettings.h>
#include <oscar/Platform/Event.h>
#include <oscar/Platform/IconCodepoints.h>
#include <oscar/Platform/Log.h>
#include <oscar/Platform/os.h>
#include <oscar/Platform/ResourceLoader.h>
#include <oscar/Platform/ResourcePath.h>
#include <oscar/Shims/Cpp20/bit.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/UI/ui_graphics_backend.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/Perf.h>

#include <imgui.h>
#include <ImGuizmo.h>
#include <implot.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

using namespace osc;
namespace rgs = std::ranges;

// SDL2 BIT
//
// this was refactored from https://github.com/ocornut/imgui/blob/v1.91.1-docking/backends/imgui_impl_sdl2.cpp

// NOLINTBEGIN

// Clang warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"  // warning: implicit conversion from 'xxx' to 'float' may lose precision
#endif

// SDL
// (the multi-viewports feature requires SDL features supported from SDL 2.0.4+. SDL 2.0.5+ is highly recommended)
#include <SDL.h>
#include <SDL_syswm.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/em_js.h>
#endif

#if SDL_VERSION_ATLEAST(2,0,4) && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && !(defined(__APPLE__) && TARGET_OS_IOS) && !defined(__amigaos4__)
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    1
#else
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE    0
#endif

namespace
{
    // A handle to a single OS mouse cursor (that the UI may switch to at runtime).
    class SystemCursor final {
    public:
        SystemCursor() = default;

        explicit SystemCursor(SDL_SystemCursor id) :
            ptr_(SDL_CreateSystemCursor(id))
        {}

        explicit operator bool () const { return ptr_.get(); }

        void use()
        {
            SDL_SetCursor(ptr_.get()); // SDL function doesn't have an early out (see #6113)
        }
    private:
        struct CursorDeleter {
            void operator()(SDL_Cursor* ptr) { SDL_FreeCursor(ptr); }
        };

        std::unique_ptr<SDL_Cursor, CursorDeleter> ptr_;
    };

    // A collection of all OS mouse cursors that the UI is capable of switching to.
    class SystemCursors final {
    public:
        SystemCursors()
        {
            static_assert(std::tuple_size_v<decltype(cursors_)> == ImGuiMouseCursor_COUNT);
            cursors_[ImGuiMouseCursor_Arrow]      = SystemCursor(SDL_SYSTEM_CURSOR_ARROW);
            cursors_[ImGuiMouseCursor_TextInput]  = SystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
            cursors_[ImGuiMouseCursor_ResizeAll]  = SystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
            cursors_[ImGuiMouseCursor_ResizeNS]   = SystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
            cursors_[ImGuiMouseCursor_ResizeEW]   = SystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
            cursors_[ImGuiMouseCursor_ResizeNESW] = SystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
            cursors_[ImGuiMouseCursor_ResizeNWSE] = SystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
            cursors_[ImGuiMouseCursor_Hand]       = SystemCursor(SDL_SYSTEM_CURSOR_HAND);
            cursors_[ImGuiMouseCursor_NotAllowed] = SystemCursor(SDL_SYSTEM_CURSOR_NO);
        }

        SystemCursor& operator[](auto pos) { return cursors_[pos]; }
    private:
        std::array<SystemCursor, ImGuiMouseCursor_COUNT> cursors_;
    };

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
        std::chrono::high_resolution_clock::time_point   Time;
        std::string                                      ClipboardText;
        bool                                             WantUpdateMonitors = true;

        // Mouse handling
        Uint32                                           MouseWindowID = 0;
        int                                              MouseButtonsDown = 0;
        SystemCursors                                    MouseCursors;
        SystemCursor*                                    MouseLastCursor = nullptr;
        int                                              MouseLastLeaveFrame = 0;
        bool                                             MouseCanUseGlobalState = can_mouse_use_global_state();
        // This is hard to use/unreliable on SDL so we'll set ImGuiBackendFlags_HasMouseHoveredViewport dynamically based on state.
        bool                                             MouseCanReportHoveredViewport = can_mouse_report_hovered_viewport(MouseCanUseGlobalState);
    };
}

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
static BackendData* ImGui_ImplSDL2_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (BackendData*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

// FIXME: Note that doesn't update with DPI/Scaling change only as SDL2 doesn't have an event for it (SDL3 has).
static void ImGui_ImplSDL2_UpdateMonitors()
{
    BackendData* bd = ImGui_ImplSDL2_GetBackendData();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Monitors.resize(0);
    bd->WantUpdateMonitors = false;
    int display_count = SDL_GetNumVideoDisplays();
    for (int n = 0; n < display_count; n++)
    {
        // Warning: the validity of monitor DPI information on Windows depends on the application DPI awareness settings, which generally needs to be set in the manifest or at runtime.
        ImGuiPlatformMonitor monitor;
        SDL_Rect r;
        SDL_GetDisplayBounds(n, &r);
        monitor.MainPos = monitor.WorkPos = ImVec2((float)r.x, (float)r.y);
        monitor.MainSize = monitor.WorkSize = ImVec2((float)r.w, (float)r.h);
#if SDL_HAS_USABLE_DISPLAY_BOUNDS
        SDL_GetDisplayUsableBounds(n, &r);
        monitor.WorkPos = ImVec2((float)r.x, (float)r.y);
        monitor.WorkSize = ImVec2((float)r.w, (float)r.h);
#endif
#if SDL_HAS_PER_MONITOR_DPI
        // FIXME-VIEWPORT: On MacOS SDL reports actual monitor DPI scale, ignoring OS configuration. We may want to set
        //  DpiScale to cocoa_window.backingScaleFactor here.
        float dpi = 0.0f;
        if (!SDL_GetDisplayDPI(n, &dpi, nullptr, nullptr))
            monitor.DpiScale = dpi / 96.0f;
#endif
        monitor.PlatformHandle = (void*)(intptr_t)n;
        platform_io.Monitors.push_back(monitor);
    }
}

// Functions
static const char* ImGui_ImplSDL2_GetClipboardText(void*)
{
    BackendData* bd = ImGui_ImplSDL2_GetBackendData();
    bd->ClipboardText = get_clipboard_text();
    return bd->ClipboardText.c_str();
}

static void ImGui_ImplSDL2_SetClipboardText(void*, const char* text)
{
    set_clipboard_text(text);
}

// Note: native IME will only display if user calls SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1") _before_ SDL_CreateWindow().
static void ImGui_ImplSDL2_PlatformSetImeData(ImGuiContext*, ImGuiViewport*, ImGuiPlatformImeData* data)
{
    if (data->WantVisible) {
        const SDL_Rect r{
            .x = (int)data->InputPos.x,
            .y = (int)data->InputPos.y,
            .w = 1,
            .h = (int)data->InputLineHeight,
        };
        SDL_SetTextInputRect(&r);
    }
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
// If you have multiple SDL events and some of them are not meant to be used by dear imgui, you may need to filter events based on their windowID field.
static bool ImGui_ImplSDL2_ProcessEvent(const Event& e)
{
    ImGuiIO& io = ImGui::GetIO();
    BackendData* bd = ImGui_ImplSDL2_GetBackendData();

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
            break;
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
    default: {
        break;
    }
    }

    switch (const SDL_Event* event = &static_cast<const SDL_Event&>(e); event->type) {
    case SDL_WINDOWEVENT:
    {
        // - When capturing mouse, SDL will send a bunch of conflicting LEAVE/ENTER event on every mouse move, but the final ENTER tends to be right.
        // - However we won't get a correct LEAVE event for a captured window.
        // - In some cases, when detaching a window from main viewport SDL may send SDL_WINDOWEVENT_ENTER one frame too late,
        //   causing SDL_WINDOWEVENT_LEAVE on previous frame to interrupt drag operation by clear mouse position. This is why
        //   we delay process the SDL_WINDOWEVENT_LEAVE events by one frame. See issue #5012 for details.
        Uint8 window_event = event->window.event;
        if (window_event == SDL_WINDOWEVENT_ENTER)
        {
            bd->MouseWindowID = event->window.windowID;
            bd->MouseLastLeaveFrame = 0;
        }
        if (window_event == SDL_WINDOWEVENT_LEAVE)
            bd->MouseLastLeaveFrame = ImGui::GetFrameCount() + 1;
        if (window_event == SDL_WINDOWEVENT_FOCUS_GAINED)
            io.AddFocusEvent(true);
        else if (window_event == SDL_WINDOWEVENT_FOCUS_LOST)
            io.AddFocusEvent(false);
        if (window_event == SDL_WINDOWEVENT_CLOSE || window_event == SDL_WINDOWEVENT_MOVED || window_event == SDL_WINDOWEVENT_RESIZED)
            if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void*)SDL_GetWindowFromID(event->window.windowID)))
            {
                if (window_event == SDL_WINDOWEVENT_CLOSE)
                    viewport->PlatformRequestClose = true;
                if (window_event == SDL_WINDOWEVENT_MOVED)
                    viewport->PlatformRequestMove = true;
                if (window_event == SDL_WINDOWEVENT_RESIZED)
                    viewport->PlatformRequestResize = true;
                return true;
            }
        return true;
    }
    }
    return false;
}

#ifdef __EMSCRIPTEN__
EM_JS(void, ImGui_ImplSDL2_EmscriptenOpenURL, (char const* url), { url = url ? UTF8ToString(url) : null; if (url) window.open(url, '_blank'); });
#endif

static void ImGui_ImplSDL2_Init(SDL_Window* window)
{
    ImGuiIO& io = ImGui::GetIO();
    OSC_ASSERT_ALWAYS(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

    // init `BackendData` and setup `ImGuiIO` pointers etc.
    io.BackendPlatformUserData = static_cast<void*>(new BackendData{window});
    io.BackendPlatformName = "imgui_impl_sdl2";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;           // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;            // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.SetClipboardTextFn = ImGui_ImplSDL2_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplSDL2_GetClipboardText;
    io.ClipboardUserData = nullptr;
    io.PlatformSetImeDataFn = ImGui_ImplSDL2_PlatformSetImeData;
#ifdef __EMSCRIPTEN__
    io.PlatformOpenInShellFn = [](ImGuiContext*, const char* url) { ImGui_ImplSDL2_EmscriptenOpenURL(url); return true; };
#endif

    // init `ImGuiViewport` for main viewport
    //
    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void*)window;
    main_viewport->PlatformHandleRaw = nullptr;
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
        main_viewport->PlatformHandleRaw = (void*)info.info.win.window;
#elif defined(__APPLE__) && defined(SDL_VIDEO_DRIVER_COCOA)
        main_viewport->PlatformHandleRaw = (void*)info.info.cocoa.window;
#endif
    }
}

static void ImGui_ImplSDL2_Shutdown()
{
    BackendData* bd = ImGui_ImplSDL2_GetBackendData();
    OSC_ASSERT_ALWAYS(bd != nullptr && "No platform backend to shutdown, or already shutdown?");
    delete bd;

    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_HasMouseHoveredViewport);
}

// This code is incredibly messy because some of the functions we need for full viewport support are not available in SDL < 2.0.4.
static void ImGui_ImplSDL2_UpdateMouseData()
{
    BackendData* bd = ImGui_ImplSDL2_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();

    // We forward mouse input when hovered or captured (via SDL_MOUSEMOTION) or when focused (below)
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    // SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the SDL window boundaries shouldn't e.g. trigger other operations outside
    SDL_CaptureMouse((bd->MouseButtonsDown != 0) ? SDL_TRUE : SDL_FALSE);
    SDL_Window* focused_window = SDL_GetKeyboardFocus();
    const bool is_app_focused = (focused_window && (bd->Window == focused_window || ImGui::FindViewportByPlatformHandle((void*)focused_window)));
#else
    SDL_Window* focused_window = bd->Window;
    const bool is_app_focused = (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_INPUT_FOCUS) != 0; // SDL 2.0.3 and non-windowed systems: single-viewport only
#endif

    if (is_app_focused)
    {
        // (Optional) Set OS mouse position from Dear ImGui if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
        if (io.WantSetMousePos)
        {
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                SDL_WarpMouseGlobal((int)io.MousePos.x, (int)io.MousePos.y);
            else
#endif
                SDL_WarpMouseInWindow(bd->Window, (int)io.MousePos.x, (int)io.MousePos.y);
        }

        // (Optional) Fallback to provide mouse position when focused (SDL_MOUSEMOTION already provides this when hovered or captured)
        if (bd->MouseCanUseGlobalState && bd->MouseButtonsDown == 0)
        {
            // Single-viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
            // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
            int mouse_x, mouse_y, window_x, window_y;
            SDL_GetGlobalMouseState(&mouse_x, &mouse_y);
            if (!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
            {
                SDL_GetWindowPosition(focused_window, &window_x, &window_y);
                mouse_x -= window_x;
                mouse_y -= window_y;
            }
            io.AddMousePosEvent((float)mouse_x, (float)mouse_y);
        }
    }

    // (Optional) When using multiple viewports: call io.AddMouseViewportEvent() with the viewport the OS mouse cursor is hovering.
    // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear imGui will ignore this field and infer the information using its flawed heuristic.
    // - [!] SDL backend does NOT correctly ignore viewports with the _NoInputs flag.
    //       Some backend are not able to handle that correctly. If a backend report an hovered viewport that has the _NoInputs flag (e.g. when dragging a window
    //       for docking, the viewport has the _NoInputs flag in order to allow us to find the viewport under), then Dear ImGui is forced to ignore the value reported
    //       by the backend, and use its flawed heuristic to guess the viewport behind.
    // - [X] SDL backend correctly reports this regardless of another viewport behind focused and dragged from (we need this to find a useful drag and drop target).
    if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
    {
        ImGuiID mouse_viewport_id = 0;
        if (SDL_Window* sdl_mouse_window = SDL_GetWindowFromID(bd->MouseWindowID))
            if (ImGuiViewport* mouse_viewport = ImGui::FindViewportByPlatformHandle((void*)sdl_mouse_window))
                mouse_viewport_id = mouse_viewport->ID;
        io.AddMouseViewportEvent(mouse_viewport_id);
    }
}

static void ImGui_ImplSDL2_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;
    BackendData* bd = ImGui_ImplSDL2_GetBackendData();

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        SDL_ShowCursor(SDL_FALSE);
    }
    else
    {
        // Show OS mouse cursor
        SystemCursor& expected_cursor = bd->MouseCursors[imgui_cursor] ? bd->MouseCursors[imgui_cursor] : bd->MouseCursors[ImGuiMouseCursor_Arrow];
        if (bd->MouseLastCursor != &expected_cursor and expected_cursor) {
            expected_cursor.use();
            bd->MouseLastCursor = &expected_cursor;
        }
        SDL_ShowCursor(SDL_TRUE);
    }
}

static void ImGui_ImplSDL2_NewFrame()
{
    BackendData* bd = ImGui_ImplSDL2_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplSDL2_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    SDL_GetWindowSize(bd->Window, &w, &h);
    if (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_MINIMIZED)
        w = h = 0;
    SDL_GL_GetDrawableSize(bd->Window, &display_w, &display_h);

    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_w / static_cast<float>(w), (float)display_h / static_cast<float>(h));

    // Update monitors
    if (bd->WantUpdateMonitors)
        ImGui_ImplSDL2_UpdateMonitors();

    // Setup time step (we don't use SDL_GetTicks() because it is using millisecond resolution)
    // (Accept SDL_GetPerformanceCounter() not returning a monotonically increasing value. Happens in VMs and Emscripten, see #6189, #6114, #3644)
    auto current_time = std::chrono::high_resolution_clock::now();
    if (current_time <= bd->Time) {
        current_time = bd->Time + std::chrono::microseconds{1};
    }
    if (bd->Time.time_since_epoch() > std::chrono::seconds{0}) {
        io.DeltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - bd->Time).count();
    }
    else {
        io.DeltaTime = (float)(1.0f / 60.0f);
    }
    bd->Time = current_time;

    if (bd->MouseLastLeaveFrame && bd->MouseLastLeaveFrame >= ImGui::GetFrameCount() && bd->MouseButtonsDown == 0) {
        bd->MouseWindowID = 0;
        bd->MouseLastLeaveFrame = 0;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    // Our io.AddMouseViewportEvent() calls will only be valid when not capturing.
    // Technically speaking testing for 'bd->MouseButtonsDown == 0' would be more rygorous, but testing for payload reduces noise and potential side-effects.
    if (bd->MouseCanReportHoveredViewport && ImGui::GetDragDropPayload() == nullptr)
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
    else
        io.BackendFlags &= ~ImGuiBackendFlags_HasMouseHoveredViewport;

    ImGui_ImplSDL2_UpdateMouseData();
    ImGui_ImplSDL2_UpdateMouseCursor();
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif


// NOLINTEND


// CONTEXT BIT

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
        const ImFontConfig& config,
        ImFontAtlas& atlas,
        const ResourcePath& path,
        const ImWchar* glyph_ranges = nullptr)
    {
        const std::string base_font_data = App::slurp(path);
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

void osc::ui::context::init()
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
        if (auto v = App::settings().find_value("experimental_feature_flags/high_dpi_mode"); v and *v) {
            return App::get().main_window_dpi() / 96.0f;
        }
        else {
            return 1.0f;  // else: assume it's an unscaled 96dpi screen
        }
    }();

    {
        const std::string base_ini_data = App::slurp("imgui_base_config.ini");
        ImGui::LoadIniSettingsFromMemory(base_ini_data.data(), base_ini_data.size());

        // CARE: the reason this filepath is `static` is because ImGui requires that
        // the string outlives the ImGui context
        static const std::string s_user_imgui_ini_file_path = (App::get().user_data_directory() / "imgui.ini").string();

        ImGui::LoadIniSettingsFromDisk(s_user_imgui_ini_file_path.c_str());
        io.IniFilename = s_user_imgui_ini_file_path.c_str();
    }

    ImFontConfig base_config;
    base_config.SizePixels = dpi_scale_factor*15.0f;
    base_config.PixelSnapH = true;
    base_config.FontDataOwnedByAtlas = true;
    add_resource_as_font(base_config, *io.Fonts, "oscar/fonts/Ruda-Bold.ttf");

    // add FontAwesome icon support
    {
        ImFontConfig config = base_config;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = floor(1.5f * config.SizePixels);
        config.GlyphMaxAdvanceX = floor(1.5f * config.SizePixels);
        static constexpr auto c_icon_ranges = std::to_array<ImWchar>({ OSC_ICON_MIN, OSC_ICON_MAX, 0 });
        add_resource_as_font(config, *io.Fonts, "oscar/fonts/fa-solid-900.ttf", c_icon_ranges.data());
    }
#endif

    // init ImGui for SDL2 /w OpenGL
    ImGui_ImplSDL2_Init(App::upd().upd_underlying_window());

    // init ImGui for OpenGL
    graphics_backend::init();

    apply_dark_theme();

    // init extra parts (plotting, gizmos, etc.)
    ImPlot::CreateContext();
}

void osc::ui::context::shutdown()
{
    ImPlot::DestroyContext();

    graphics_backend::shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

bool osc::ui::context::on_event(const Event& ev)
{
    ImGui_ImplSDL2_ProcessEvent(ev);

    if (ImGui::GetIO().WantCaptureKeyboard and (ev.type() == EventType::KeyDown or ev.type() == EventType::KeyUp)) {
        return true;
    }
    if (ImGui::GetIO().WantCaptureMouse and (ev.type() == EventType::MouseWheel or ev.type() == EventType::MouseMove or ev.type() == EventType::MouseButtonUp or ev.type() == EventType::MouseButtonDown)) {
        return true;
    }
    return false;
}

void osc::ui::context::on_start_new_frame()
{
    graphics_backend::on_start_new_frame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // extra parts
    ImGuizmo::BeginFrame();
}

void osc::ui::context::render()
{
    {
        OSC_PERF("ImGuiRender/render");
        ImGui::Render();
    }

    {
        OSC_PERF("ImGuiRender/ImGui_ImplOscarGfx_RenderDrawData");
        graphics_backend::render(ImGui::GetDrawData());
    }
}

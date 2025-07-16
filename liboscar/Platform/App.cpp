#include "App.h"

#include <liboscar/Graphics/GraphicsContext.h>
#include <liboscar/Graphics/Texture2D.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Platform/AppClock.h>
#include <liboscar/Platform/AppMetadata.h>
#include <liboscar/Platform/AppSettings.h>
#include <liboscar/Platform/Cursor.h>
#include <liboscar/Platform/CursorShape.h>
#include <liboscar/Platform/Events.h>
#include <liboscar/Platform/FilesystemResourceLoader.h>
#include <liboscar/Platform/Log.h>
#include <liboscar/Platform/MouseButton.h>
#include <liboscar/Platform/PhysicalKeyModifier.h>
#include <liboscar/Platform/ResourceLoader.h>
#include <liboscar/Platform/ResourcePath.h>
#include <liboscar/Platform/ResourceStream.h>
#include <liboscar/Platform/Screenshot.h>
#include <liboscar/Platform/Widget.h>
#include <liboscar/Platform/os.h>
#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/Conversion.h>
#include <liboscar/Utils/EnumHelpers.h>
#include <liboscar/Utils/Perf.h>
#include <liboscar/Utils/ScopeExit.h>
#include <liboscar/Utils/SynchronizedValue.h>
#include <liboscar/Utils/TypeInfoReference.h>

#include <ankerl/unordered_dense.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace osc;

template<>
struct osc::Converter<SDL_Rect, Rect> final {
    Rect operator()(const SDL_Rect& rect) const
    {
        const Vec2 top_left{rect.x, rect.y};
        const Vec2 dimensions{rect.w, rect.h};
        return Rect::from_corners(top_left, top_left + dimensions);
    }
};

template<>
struct osc::Converter<Rect, SDL_Rect> final {
    SDL_Rect operator()(const Rect& ypd_rect) const
    {
        return {
            .x = static_cast<int>(ypd_rect.left()),
            .y = static_cast<int>(ypd_rect.ypd_top()),
            .w = static_cast<int>(ypd_rect.width()),
            .h = static_cast<int>(ypd_rect.height()),
        };
    }
};

PhysicalKeyModifiers osc::Converter<KeyModifiers, PhysicalKeyModifiers>::operator()(KeyModifiers modifiers) const
{
    // Ensure the remapping/casting tricks being done in this function are valid.
    static_assert(num_flags<KeyModifier>() == num_flags<PhysicalKeyModifier>());
    static_assert(num_flags<KeyModifier>() == 4);
    static_assert(std::to_underlying(KeyModifier::Shift) == std::to_underlying(PhysicalKeyModifier::Shift));
    static_assert(std::to_underlying(KeyModifier::Ctrl)  == std::to_underlying(PhysicalKeyModifier::Ctrl));
    static_assert(std::to_underlying(KeyModifier::Meta)  == std::to_underlying(PhysicalKeyModifier::Meta));
    static_assert(std::to_underlying(KeyModifier::Alt)   == std::to_underlying(PhysicalKeyModifier::Alt));
    static_assert(std::is_same_v<KeyModifiers::underlying_type, PhysicalKeyModifiers::underlying_type>);

#if defined(__APPLE__)
    // MacOS `KeyModifiers` are remapped so that application code can write
    // keybinds as-if only writing for Windows/Linux. This function un-maps
    // them.
    modifiers = modifiers.with_flag_values_swapped(KeyModifier::Ctrl, KeyModifier::Meta);
#endif

    return PhysicalKeyModifiers::from_underlying(to_underlying(modifiers));
}

KeyModifiers osc::Converter<PhysicalKeyModifiers, KeyModifiers>::operator()(PhysicalKeyModifiers modifiers) const
{
    // Ensure the remapping/casting tricks being done in this function are valid.
    static_assert(num_flags<KeyModifier>() == num_flags<PhysicalKeyModifier>());
    static_assert(num_flags<KeyModifier>() == 4);
    static_assert(std::to_underlying(KeyModifier::Shift) == std::to_underlying(PhysicalKeyModifier::Shift));
    static_assert(std::to_underlying(KeyModifier::Ctrl)  == std::to_underlying(PhysicalKeyModifier::Ctrl));
    static_assert(std::to_underlying(KeyModifier::Meta)  == std::to_underlying(PhysicalKeyModifier::Meta));
    static_assert(std::to_underlying(KeyModifier::Alt)   == std::to_underlying(PhysicalKeyModifier::Alt));
    static_assert(std::is_same_v<KeyModifiers::underlying_type, PhysicalKeyModifiers::underlying_type>);

#if defined(__APPLE__)
    // MacOS `PhysicalKeyModifiers` need to be remapped so that application code
    // can treat keybinds as-if only writing for Windows/Linux. This function maps
    // them.
    modifiers = modifiers.with_flag_values_swapped(PhysicalKeyModifier::Ctrl, PhysicalKeyModifier::Meta);
#endif

    return KeyModifiers::from_underlying(to_underlying(modifiers));
}

template<>
struct osc::Converter<Uint16, KeyModifiers> final {
    KeyModifiers operator()(Uint16 mod) const
    {
        PhysicalKeyModifiers physical_keymods;
        for (const auto& [sdl_modifier, physical_keymod] : c_mappings_) {
            if (mod & sdl_modifier) {
                physical_keymods |= physical_keymod;
            }
        }
        return to<KeyModifiers>(physical_keymods);
    }
private:
    using Mapping = std::pair<SDL_Keymod, PhysicalKeyModifier>;

    static constexpr auto c_mappings_ = std::to_array<Mapping>({
        {SDL_KMOD_LSHIFT, PhysicalKeyModifier::Shift},
        {SDL_KMOD_RSHIFT, PhysicalKeyModifier::Shift},
        {SDL_KMOD_LALT,   PhysicalKeyModifier::Alt},
        {SDL_KMOD_RALT,   PhysicalKeyModifier::Alt},
        {SDL_KMOD_LCTRL,  PhysicalKeyModifier::Ctrl},
        {SDL_KMOD_RCTRL,  PhysicalKeyModifier::Ctrl},
        {SDL_KMOD_LGUI,   PhysicalKeyModifier::Meta},
        {SDL_KMOD_RGUI,   PhysicalKeyModifier::Meta},
    });
};

template<>
struct osc::Converter<SDL_Keycode, Key> final {
    Key operator()(SDL_Keycode code) const
    {
        static_assert(num_options<Key>() == 120);

        switch (code) {
        case SDLK_TAB:          return Key::Tab;
        case SDLK_LEFT:         return Key::LeftArrow;
        case SDLK_RIGHT:        return Key::RightArrow;
        case SDLK_UP:           return Key::UpArrow;
        case SDLK_DOWN:         return Key::DownArrow;
        case SDLK_PAGEUP:       return Key::PageUp;
        case SDLK_PAGEDOWN:     return Key::PageDown;
        case SDLK_HOME:         return Key::Home;
        case SDLK_END:          return Key::End;
        case SDLK_INSERT:       return Key::Insert;
        case SDLK_DELETE:       return Key::Delete;
        case SDLK_BACKSPACE:    return Key::Backspace;
        case SDLK_SPACE:        return Key::Space;
        case SDLK_RETURN:       return Key::Return;
        case SDLK_ESCAPE:       return Key::Escape;
        case SDLK_APOSTROPHE:   return Key::Apostrophe;
        case SDLK_COMMA:        return Key::Comma;
        case SDLK_MINUS:        return Key::Minus;
        case SDLK_PERIOD:       return Key::Period;
        case SDLK_SLASH:        return Key::Slash;
        case SDLK_SEMICOLON:    return Key::Semicolon;
        case SDLK_EQUALS:       return Key::Equals;
        case SDLK_LEFTBRACKET:  return Key::LeftBracket;
        case SDLK_BACKSLASH:    return Key::Backslash;
        case SDLK_RIGHTBRACKET: return Key::RightBracket;
        case SDLK_GRAVE:        return Key::Grave;
        case SDLK_CAPSLOCK:     return Key::CapsLock;
        case SDLK_SCROLLLOCK:   return Key::ScrollLock;
        case SDLK_NUMLOCKCLEAR: return Key::NumLockClear;
        case SDLK_PRINTSCREEN:  return Key::PrintScreen;
        case SDLK_PAUSE:        return Key::Pause;
        case SDLK_KP_0:         return Key::Keypad0;
        case SDLK_KP_1:         return Key::Keypad1;
        case SDLK_KP_2:         return Key::Keypad2;
        case SDLK_KP_3:         return Key::Keypad3;
        case SDLK_KP_4:         return Key::Keypad4;
        case SDLK_KP_5:         return Key::Keypad5;
        case SDLK_KP_6:         return Key::Keypad6;
        case SDLK_KP_7:         return Key::Keypad7;
        case SDLK_KP_8:         return Key::Keypad8;
        case SDLK_KP_9:         return Key::Keypad9;
        case SDLK_KP_PERIOD:    return Key::KeypadPeriod;
        case SDLK_KP_DIVIDE:    return Key::KeypadDivide;
        case SDLK_KP_MULTIPLY:  return Key::KeypadMultiply;
        case SDLK_KP_MINUS:     return Key::KeypadMinus;
        case SDLK_KP_PLUS:      return Key::KeypadPlus;
        case SDLK_KP_ENTER:     return Key::KeypadEnter;
        case SDLK_KP_EQUALS:    return Key::KeypadEquals;
        case SDLK_LCTRL:        return Key::LeftCtrl;
        case SDLK_LSHIFT:       return Key::LeftShift;
        case SDLK_LALT:         return Key::LeftAlt;
        case SDLK_LGUI:         return Key::LeftGui;
        case SDLK_RCTRL:        return Key::RightCtrl;
        case SDLK_RSHIFT:       return Key::RightShift;
        case SDLK_RALT:         return Key::RightAlt;
        case SDLK_RGUI:         return Key::RightGui;
        case SDLK_APPLICATION:  return Key::Application;
        case SDLK_0:            return Key::_0;
        case SDLK_1:            return Key::_1;
        case SDLK_2:            return Key::_2;
        case SDLK_3:            return Key::_3;
        case SDLK_4:            return Key::_4;
        case SDLK_5:            return Key::_5;
        case SDLK_6:            return Key::_6;
        case SDLK_7:            return Key::_7;
        case SDLK_8:            return Key::_8;
        case SDLK_9:            return Key::_9;
        case SDLK_A:            return Key::A;
        case SDLK_B:            return Key::B;
        case SDLK_C:            return Key::C;
        case SDLK_D:            return Key::D;
        case SDLK_E:            return Key::E;
        case SDLK_F:            return Key::F;
        case SDLK_G:            return Key::G;
        case SDLK_H:            return Key::H;
        case SDLK_I:            return Key::I;
        case SDLK_J:            return Key::J;
        case SDLK_K:            return Key::K;
        case SDLK_L:            return Key::L;
        case SDLK_M:            return Key::M;
        case SDLK_N:            return Key::N;
        case SDLK_O:            return Key::O;
        case SDLK_P:            return Key::P;
        case SDLK_Q:            return Key::Q;
        case SDLK_R:            return Key::R;
        case SDLK_S:            return Key::S;
        case SDLK_T:            return Key::T;
        case SDLK_U:            return Key::U;
        case SDLK_V:            return Key::V;
        case SDLK_W:            return Key::W;
        case SDLK_X:            return Key::X;
        case SDLK_Y:            return Key::Y;
        case SDLK_Z:            return Key::Z;
        case SDLK_F1:           return Key::F1;
        case SDLK_F2:           return Key::F2;
        case SDLK_F3:           return Key::F3;
        case SDLK_F4:           return Key::F4;
        case SDLK_F5:           return Key::F5;
        case SDLK_F6:           return Key::F6;
        case SDLK_F7:           return Key::F7;
        case SDLK_F8:           return Key::F8;
        case SDLK_F9:           return Key::F9;
        case SDLK_F10:          return Key::F10;
        case SDLK_F11:          return Key::F11;
        case SDLK_F12:          return Key::F12;
        case SDLK_F13:          return Key::F13;
        case SDLK_F14:          return Key::F14;
        case SDLK_F15:          return Key::F15;
        case SDLK_F16:          return Key::F16;
        case SDLK_F17:          return Key::F17;
        case SDLK_F18:          return Key::F18;
        case SDLK_F19:          return Key::F19;
        case SDLK_F20:          return Key::F20;
        case SDLK_F21:          return Key::F21;
        case SDLK_F22:          return Key::F22;
        case SDLK_F23:          return Key::F23;
        case SDLK_F24:          return Key::F24;
        case SDLK_AC_BACK:      return Key::AppBack;
        case SDLK_AC_FORWARD:   return Key::AppForward;
        default:                return Key::Unknown;
        }
    }
};

template<>
struct osc::Converter<Uint8, osc::MouseButton> final {
    osc::MouseButton operator()(Uint8 sdlval) const
    {
        switch (sdlval) {
        case SDL_BUTTON_LEFT:   return MouseButton::Left;
        case SDL_BUTTON_RIGHT:  return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_X1:     return MouseButton::Back;
        case SDL_BUTTON_X2:     return MouseButton::Forward;
        default:                return MouseButton::None;
        }
    }
};

namespace osc::sdl
{
    // RAII wrapper for `SDL_Init` and `SDL_Quit`
    //     https://wiki.libsdl.org/SDL_Quit
    class Context final {
    public:
        explicit Context(Uint32 flags)
        {
            if (not SDL_Init(flags)) {
                throw std::runtime_error{std::string{"SDL_Init: failed: "} + SDL_GetError()};
            }
        }
        Context(const Context&) = delete;
        Context(Context&&) noexcept = delete;
        Context& operator=(const Context&) = delete;
        Context& operator=(Context&&) noexcept = delete;
        ~Context() noexcept
        {
            SDL_Quit();
        }
    };

    // https://wiki.libsdl.org/SDL_Init
    inline Context Init(Uint32 flags)
    {
        return Context{flags};
    }

    // RAII wrapper around `SDL_Window` that calls `SDL_DestroyWindow` on dtor
    //     https://wiki.libsdl.org/SDL_CreateWindow
    //     https://wiki.libsdl.org/SDL_DestroyWindow
    class Window final {
    public:
        explicit Window(SDL_Window * _ptr) :
            window_handle_{_ptr}
        {}
        Window(const Window&) = delete;
        Window(Window&& tmp) noexcept :
            window_handle_{std::exchange(tmp.window_handle_, nullptr)}
        {}
        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) noexcept = delete;
        ~Window() noexcept
        {
            if (window_handle_) {
                SDL_DestroyWindow(window_handle_);
            }
        }

        SDL_Window* get() const { return window_handle_; }

        SDL_Window& operator*() const { return *window_handle_; }

    private:
        SDL_Window* window_handle_;
    };
}

namespace
{
    App* g_app_global = nullptr;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#ifndef EMSCRIPTEN
    void sdl_gl_set_attribute_or_throw(
        SDL_GLAttr attribute,
        CStringView attribute_readable_name,
        int new_attribute_value,
        CStringView value_readable_name)
    {
        if (not SDL_GL_SetAttribute(attribute, new_attribute_value)) {
            std::stringstream msg;
            msg << "SDL_GL_SetAttribute failed when setting " << attribute_readable_name << " = " << value_readable_name << ": " << SDL_GetError();
            throw std::runtime_error{std::move(msg).str()};
        }
    }
#endif

    LogLevel get_log_level_from_settings(const AppSettings& settings)
    {
        if (const auto v = settings.find_value("log_level")) {
            if (auto parsed = try_parse_as_log_level(to<std::string>(*v))) {
                return *parsed;
            }
        }
        return LogLevel::DEFAULT;
    }

    bool configure_application_log(const AppSettings& config)
    {
        if (auto logger = global_default_logger()) {
            logger->set_level(get_log_level_from_settings(config));
        }
        return true;
    }

    // initialize the main application window
    sdl::Window create_main_app_window(const AppSettings&, CStringView application_name)
    {
        log_info("initializing main application window");

#ifndef EMSCRIPTEN
        // note: cannot set context attributes in EMSCRIPTEN
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_PROFILE_MASK, "SDL_GL_CONTEXT_PROFILE_MASK", SDL_GL_CONTEXT_PROFILE_CORE, "SDL_GL_CONTEXT_PROFILE_CORE");
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_MAJOR_VERSION, "SDL_GL_CONTEXT_MAJOR_VERSION", 3, "3");
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_MINOR_VERSION, "SDL_GL_CONTEXT_MINOR_VERSION", 3, "3");
        sdl_gl_set_attribute_or_throw(SDL_GL_CONTEXT_FLAGS, "SDL_GL_CONTEXT_FLAGS", SDL_GL_CONTEXT_DEBUG_FLAG, "SDL_GL_CONTEXT_DEBUG_FLAG");
        sdl_gl_set_attribute_or_throw(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, "SDL_GL_FRAMEBUFFER_SRGB_CAPABLE", 1, "1");
#endif

        // adapted from: https://github.com/ocornut/imgui/blob/v1.91.1-docking/backends/imgui_impl_sdl2.cpp
        //
        // From 2.0.5: Set SDL hint to receive mouse click events on window focus, otherwise SDL doesn't emit the event.
        // Without this, when clicking to gain focus, our widgets wouldn't activate even though they showed as hovered.
        // (This is unfortunately a global SDL setting, so enabling it might have a side-effect on your application.
        // It is unlikely to make a difference, but if your app absolutely needs to ignore the initial on-focus click:
        // you can ignore SDL_MOUSEBUTTONDOWN events coming right after a SDL_WINDOWEVENT_FOCUS_GAINED)
#ifdef SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH
        SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
#endif

        // adapted from: https://github.com/ocornut/imgui/blob/v1.91.1-docking/backends/imgui_impl_sdl2.cpp
        //
        // From 2.0.18: Enable native IME.
        // IMPORTANT: This is used at the time of SDL_CreateWindow() so this will only affects secondary windows, if any.
        // For the main window to be affected, your application needs to call this manually before calling SDL_CreateWindow().
#ifdef SDL_HINT_IME_SHOW_UI
        SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

        // adapted from: https://github.com/ocornut/imgui/blob/v1.91.1-docking/backends/imgui_impl_sdl2.cpp
        //
        // From 2.0.22: Disable auto-capture, this is preventing drag and drop across multiple windows (see #5710)
#ifdef SDL_HINT_MOUSE_AUTO_CAPTURE
        SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");
#endif
        SDL_PropertiesID properties = SDL_CreateProperties();
        const ScopeExit g{[&]{ SDL_DestroyProperties(properties); }};

        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, true);
        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_HIGH_PIXEL_DENSITY_BOOLEAN, true);
        SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, application_name.c_str());
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 800);
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 600);
        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, SDL_getenv_unsafe("OSC_INTERNAL_HIDE_WINDOW") != nullptr);

        SDL_Window* const rv = SDL_CreateWindowWithProperties(properties);
        if (rv == nullptr) {
            throw std::runtime_error{std::string{"SDL_CreateWindow failed: "} + SDL_GetError()};
        }

        return sdl::Window{rv};
    }

    AppClock::duration convert_perf_ticks_to_appclock_duration(Uint64 ticks, Uint64 frequency)
    {
        const auto dticks = static_cast<double>(ticks);
        const auto dfrequency = static_cast<double>(frequency);
        const auto duration = static_cast<AppClock::rep>(dticks/dfrequency);

        return AppClock::duration{duration};
    }

    AppClock::time_point convert_perf_counter_to_appclock(Uint64 ticks, Uint64 frequency)
    {
        return AppClock::time_point{convert_perf_ticks_to_appclock_duration(ticks, frequency)};
    }

    std::filesystem::path get_current_exe_dir_and_log_it()
    {
        auto rv = current_executable_directory();
        log_info("executable directory: %s", rv.string().c_str());
        return rv;
    }

    // computes the user's data directory and also logs it to the console for user-facing feedback
    std::filesystem::path get_current_user_dir_and_log_it(
        std::string_view organization_name,
        std::string_view application_name)
    {
        auto rv = user_data_directory(organization_name, application_name);
        log_info("user data directory: %s", rv.string().c_str());
        return rv;
    }

    WindowEventType parse_as_window_event_type(Uint32 t)
    {
        switch (t) {
        case SDL_EVENT_WINDOW_MOUSE_ENTER:           return WindowEventType::GainedMouseFocus;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:           return WindowEventType::LostMouseFocus;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:          return WindowEventType::GainedKeyboardFocus;
        case SDL_EVENT_WINDOW_FOCUS_LOST:            return WindowEventType::LostKeyboardFocus;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:       return WindowEventType::WindowClosed;
        case SDL_EVENT_WINDOW_MOVED:                 return WindowEventType::WindowMoved;
        case SDL_EVENT_WINDOW_RESIZED:               return WindowEventType::WindowResized;
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: return WindowEventType::WindowDisplayScaleChanged;
        default:                                     return WindowEventType::Unknown;
        }
    }

    std::unique_ptr<Event> try_parse_into_event(
        const SDL_Event& e,
        Vec2 main_window_dimensions,
        const std::function<float()>& os_to_main_window_device_independent_ratio_getter)
    {
        if (e.type == SDL_EVENT_DROP_FILE and e.drop.data) {
            return std::make_unique<DropFileEvent>(std::filesystem::path{e.drop.data});
        }
        else if (e.type == SDL_EVENT_KEY_DOWN) {
            return std::make_unique<KeyEvent>(KeyEvent::key_down(KeyCombination{to<KeyModifiers>(e.key.mod), to<Key>(e.key.key)}));
        }
        else if (e.type == SDL_EVENT_KEY_UP) {
            return std::make_unique<KeyEvent>(KeyEvent::key_up(KeyCombination{to<KeyModifiers>(e.key.mod), to<Key>(e.key.key)}));
        }
        else if (e.type == SDL_EVENT_QUIT) {
            return std::make_unique<QuitEvent>();
        }
        else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            const MouseInputSource source = e.button.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse;
            return std::make_unique<MouseEvent>(MouseEvent::button_down(source, to<MouseButton>(e.button.button)));
        }
        else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            const MouseInputSource source = e.button.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse;
            return std::make_unique<MouseEvent>(MouseEvent::button_up(source, to<MouseButton>(e.button.button)));
        }
        else if (e.type == SDL_EVENT_MOUSE_MOTION) {
            const MouseInputSource source = e.motion.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse;
            const float os_to_main_window_device_independent_ratio = os_to_main_window_device_independent_ratio_getter();

            Vec2 relative_delta = {static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel)};
            relative_delta *= os_to_main_window_device_independent_ratio;            // convert SDL3 units (pixels) to device-independent pixels
            relative_delta.y = main_window_dimensions.y - relative_delta.y;          // convert from SDL3 space (top-left origin, left-handed) to screen space

            Vec2 position_in_window = {static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
            position_in_window *= os_to_main_window_device_independent_ratio;        // convert SDL3 units (pixels) to device-independent pixels
            position_in_window.y = main_window_dimensions.y - position_in_window.y;  // convert from SDL3 space (top-left origin, left-handed) to screen space

            return std::make_unique<MouseEvent>(MouseEvent::motion(source, relative_delta, position_in_window));
        }
        else if (e.type == SDL_EVENT_MOUSE_WHEEL) {
            Vec2 delta = {e.wheel.x, e.wheel.y};
            const MouseInputSource source = e.wheel.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse;
            if (source == MouseInputSource::Mouse) {
                // Normalize mouse inputs such that each "click" of the mouse maps to -1 or +1
                //
                // The reason to do this is because different operating systems have different
                // orders of magnitude and frequency for scroll events, so this section needs
                // to try and hide that fact (MacOS, in particular, reports completely different
                // raw deltas from other OSes #971).
                if (delta.x != 0.0f) {
                    delta.x = delta.x > 0.0f ? +1.0f : -1.0f;
                }
                if (delta.y != 0.0f) {
                    delta.y = delta.y > 0.0f ? +1.0f : -1.0f;
                }
            }
            return std::make_unique<MouseWheelEvent>(delta, source);
        }
        else if (e.type == SDL_EVENT_TEXT_INPUT) {
            return std::make_unique<TextInputEvent>(std::string{static_cast<const char*>(e.text.text)});
        }
        else if (SDL_EVENT_DISPLAY_FIRST <= e.type and e.type <=  SDL_EVENT_DISPLAY_LAST) {
            return std::make_unique<DisplayStateChangeEvent>();
        }
        else if (SDL_EVENT_WINDOW_FIRST <= e.type and e.type <= SDL_EVENT_WINDOW_LAST) {
            static_assert(num_options<WindowEventType>() == 9);
            OSC_ASSERT(SDL_EVENT_WINDOW_FIRST <= e.type and e.type <= SDL_EVENT_WINDOW_LAST);

            const WindowEventType type = parse_as_window_event_type(e.type);
            const WindowID id{SDL_GetWindowFromID(e.window.windowID)};
            const uint32_t window_id = e.window.windowID;
            return std::make_unique<WindowEvent>(type, id, window_id);
        }
        else {
            return nullptr;
        }
    }
}

namespace
{
    // an "active" request for an annotated screenshot
    //
    // has a data dependency on the backend first providing a "raw" image, which
    // is then tagged with annotations
    struct AnnotatedScreenshotRequest final {

        AnnotatedScreenshotRequest(
            size_t frame_requested_,
            std::future<Texture2D> underlying_future_) :

            frame_requested{frame_requested_},
            underlying_future{std::move(underlying_future_)}
        {}

        // the frame on which the screenshot was requested
        size_t frame_requested;

        // underlying (to-be-waited-on) future for the screenshot
        std::future<Texture2D> underlying_future;

        // our promise to the caller, who is waiting for an annotated image
        std::promise<Screenshot> result_promise;

        // annotations made during the requested frame (if any)
        std::vector<ScreenshotAnnotation> annotations;
    };

    // A handle to a single OS mouse cursor (that the UI may switch to at runtime).
    class SystemCursor final {
    public:
        SystemCursor() = default;

        explicit SystemCursor(SDL_SystemCursor id) :
            ptr_(SDL_CreateSystemCursor(id))
        {}

        explicit operator bool () const { return static_cast<bool>(ptr_); }

        SDL_Cursor* get() { return ptr_.get(); }
    private:
        struct CursorDeleter {
            void operator()(SDL_Cursor* ptr) { SDL_DestroyCursor(ptr); }
        };

        std::unique_ptr<SDL_Cursor, CursorDeleter> ptr_;
    };

    // A collection of all OS mouse cursors that the UI is capable of switching to.
    class SystemCursors final {
    public:
        SystemCursor& operator[](CursorShape shape) { return cursors_.at(to_index(shape)); }
    private:
        std::array<SystemCursor, num_options<CursorShape>()> cursors_ = std::to_array({
            SystemCursor(SDL_SYSTEM_CURSOR_DEFAULT),      // CursorShape::Arrow
            SystemCursor(SDL_SYSTEM_CURSOR_TEXT),         // CursorShape::IBeam
            SystemCursor(SDL_SYSTEM_CURSOR_MOVE),         // CursorShape::ResizeAll
            SystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE),    // CursorShape::ResizeVertical
            SystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE),    // CursorShape::ResizeHorizontal
            SystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE),  // CursorShape::ResizeDiagonalNESW
            SystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE),  // CursorShape::ResizeDiagonalNWSE
            SystemCursor(SDL_SYSTEM_CURSOR_POINTER),      // CursorShape::PointingHand
            SystemCursor(SDL_SYSTEM_CURSOR_WAIT),         // CursorShape::Wait
            SystemCursor(SDL_SYSTEM_CURSOR_PROGRESS),     // CursorShape::Progress
            SystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED),  // CursorShape::Forbidden
            SystemCursor{},                               // CursorShape::Hidden
        });
    };

    class CursorHandler final {
    public:
        CursorHandler()
        {
            push_cursor_override(Cursor{CursorShape::Forbidden});  // initialize sentinel
        }
        CursorHandler(const CursorHandler&) = delete;
        CursorHandler(CursorHandler&&) = delete;
        CursorHandler& operator=(const CursorHandler&) = delete;
        CursorHandler& operator=(CursorHandler&&) = delete;
        ~CursorHandler() noexcept
        {
            // try to reset the cursor to default
            if (cursor_stack_.size() > 1) {
                SDL_ShowCursor();
                SDL_SetCursor(system_mouse_cursors_[CursorShape::Arrow].get());
            }
        }

        void push_cursor_override(const Cursor& cursor)
        {
            if (cursor.shape() != CursorShape::Hidden) {
                SDL_ShowCursor();
            }
            else {
                SDL_HideCursor();
            }
            SDL_SetCursor(system_mouse_cursors_[cursor.shape()].get());
            cursor_stack_.push_back(cursor.shape());
        }

        void pop_cursor_override()
        {
            // note: there's a sentinel cursor at the bottom of the stack that's initialized
            //       by the constructor
            OSC_ASSERT(cursor_stack_.size() > 1 && "tried to call App::pop_cursor_override when no cursor overrides were pushed");

            cursor_stack_.pop_back();
            SDL_SetCursor(system_mouse_cursors_[cursor_stack_.back()].get());
            if (cursor_stack_.empty() or cursor_stack_.back() != CursorShape::Hidden) {
                SDL_ShowCursor();
            }
            else {
                SDL_HideCursor();
            }
        }
    private:
        // runtime lookup of all available mouse cursors
        SystemCursors system_mouse_cursors_;

        // current stack of application-level cursor overrides
        std::vector<CursorShape> cursor_stack_;
    };

    // Internal event that's used by the application to marshal a callback to
    // the main thread.
    class AppMarshalledCallbackEvent final : public Event {
    public:
        explicit AppMarshalledCallbackEvent(std::function<void()> callback) :
            Event{EventType::Custom},
            callback_{std::move(callback)}
        {}

        void invoke_callback() { callback_(); }
    private:
        std::function<void()> callback_;
    };

    // State that's stored in the sdl3 callback when using a file dialog.
    struct SDL3DialogCallbackState final {

        // This free function is what SDL calls with `SDL3CallbackState` when the user is
        // finished with the dialog.
        static void sdl3_compatible_callback(void* userdata, const char* const* filelist, int)
        {
            // Unpack callback state.
            const std::unique_ptr<SDL3DialogCallbackState> state{static_cast<SDL3DialogCallbackState*>(userdata)};

            // If there's an error, emit a `FileDialogResponse` that contains the error.
            if (not filelist) {
                App::upd().request_invoke_on_main_thread([caller_callback = std::move(state->caller_callback), response = FileDialogResponse{SDL_GetError()}]() mutable
                {
                    caller_callback(std::move(response));
                });
                return;
            }

            // Convert SDL's file list to an oscar `FileDialogResponse`
            std::vector<std::filesystem::path> files;
            while (*filelist) {
                files.emplace_back(*filelist);
                ++filelist;
            }

            // Marshal the call to the user's callback onto the main thread by packing it
            // into an `AppMarshalledCallbackEvent`.
            App::upd().request_invoke_on_main_thread([caller_callback = std::move(state->caller_callback), response = FileDialogResponse{std::move(files)}]() mutable
            {
                // Call the user's callback (the event's callback happens on the main thread).
                caller_callback(std::move(response));
            });
        }

        // Constructs the callback state that's stored in SDL3's dialog system.
        explicit SDL3DialogCallbackState(
            std::function<void(FileDialogResponse&&)>&& callback_,
            std::span<const FileDialogFilter> filters_ = {}) :
            caller_callback{std::move(callback_)},
            caller_filters(filters_.begin(), filters_.end())
        {
            // The caller's filters are lifetime-controlled (`std::string`s), the SDL
            // filters are not-lifetime-controlled views (`const char*`s). The SDL3
            // API for `SDL_ShowOpenFileDialog` mandates that "the filters' data
            // must be valid at least until `sdl3_callback` is called", so we keep
            // both alive.
            sdl3_filters.reserve(caller_filters.size());
            for (const FileDialogFilter& filter : caller_filters) {
                sdl3_filters.push_back(SDL_DialogFileFilter{
                    .name = filter.name().c_str(),
                    .pattern = filter.pattern().c_str(),
                });
            }
        }

        std::function<void(FileDialogResponse&&)> caller_callback;
        std::vector<FileDialogFilter> caller_filters;
        std::vector<SDL_DialogFileFilter> sdl3_filters;
    };
}

// main application state
//
// this is what "booting the application" actually initializes
class osc::App::Impl final {
public:
    explicit Impl(const AppMetadata& metadata) :  // NOLINT(modernize-pass-by-value)
        metadata_{metadata}
    {}

    const AppMetadata& metadata() const { return metadata_; }
    std::string human_readable_name() const { return std::string{metadata().human_readable_application_name()}; }
    std::string application_name_with_version_and_buildid() const { return metadata().application_name_with_version_and_buildid(); }
    const std::filesystem::path& executable_directory() const { return executable_dir_; }
    const std::filesystem::path& user_data_directory() const { return user_data_dir_; }

    void setup_main_loop(std::unique_ptr<Widget> widget)
    {
        if (current_widget_) {
            throw std::runtime_error{"tried to call `App::setup_main_loop` when a widget is already being shown (and, therefore, `App::teardown_main_loop` wasn't called). If you want to change the application's top-level widget from *within* some other widget, call `request_transition` instead"};
        }

        log_info("initializing application main loop with widget '%s'", widget->name().c_str());

        // reset loop-dependent state variables
        perf_counter_ = SDL_GetPerformanceCounter();
        frame_counter_ = 0;
        frame_start_time_ = convert_perf_counter_to_appclock(perf_counter_, perf_counter_frequency_);
        time_since_last_frame_ = AppClock::duration{1.0f/60.0f};  // (dummy value for the first frame)
        quit_requested_ = false;
        is_in_wait_mode_ = false;
        num_frames_to_poll_ = 2;

        // perform initial top-level widget mount
        current_widget_ = std::move(widget);
        current_widget_->on_mount();
    }

    AppMainLoopStatus do_main_loop_step()
    {
        // pump events
        {
            OSC_PERF("App/pump_events");

            bool should_wait = is_in_wait_mode_ and num_frames_to_poll_ <= 0;
            num_frames_to_poll_ = max(0, num_frames_to_poll_ - 1);

            for (SDL_Event e; should_wait ? SDL_WaitEventTimeout(&e, 1000) : SDL_PollEvent(&e);) {
                should_wait = false;

                // edge-case: it's an `SDL_EVENT_USER`:
                //
                // - `SDL_EVENT_USER`'s are only launched from this compilation unit (search for it)
                // - They're either:
                //   - A custom event posted to a `Widget`
                //   - A custom event posted to "the top-level application". The only situation where
                //     that's permitted is thread marshalling.
                //   - A "blank" event, used to crank the event loop and make the application redraw
                //     the top-level widget
                if (e.type == SDL_EVENT_USER) {
                    if (e.user.data1 and e.user.data2) {
                        // a custom event posted to a `Widget`
                        auto* receiver_widget = static_cast<Widget*>(e.user.data1);
                        auto custom_event = std::unique_ptr<Event>(static_cast<Event*>(e.user.data2));
                        notify(*receiver_widget, *custom_event);
                        continue;  // event handled - go get the next one
                    }
                    else if (not e.user.data1 and e.user.data2) {
                        // a custom event posted to "the top-level application" (thread marshalling)
                        auto custom_event = std::unique_ptr<Event>(static_cast<Event*>(e.user.data2));
                        if (auto* marshalled_event = dynamic_cast<AppMarshalledCallbackEvent*>(custom_event.get())) {
                            marshalled_event->invoke_callback();
                        }
                        continue;  // assume event is handled
                    }
                    else {
                        // a "blank" event (`request_redraw`)
                        continue;
                    }
                }

                // let top-level widget handle the event
                bool widget_handled_event = false;
                if (auto parsed = try_parse_into_event(e, main_window_dimensions(), [this]{ return os_to_main_window_device_independent_ratio(); })) {
                    widget_handled_event = current_widget_->on_event(*parsed);
                }

                // if the current widget didn't handle the event, try to handle it here by following
                // reasonable heuristics
                if (not widget_handled_event) {
                    if (SDL_EVENT_WINDOW_FIRST <= e.type and e.type <= SDL_EVENT_WINDOW_LAST) {
                        // window was resized and should be drawn a couple of times quickly
                        // to ensure the current top-level widget has a chance to reflow etc.
                        num_frames_to_poll_ = 2;
                    }
                    else if (e.type == SDL_EVENT_QUIT) {
                        request_quit();  // i.e. "as if the top-level widget tried to quit"
                    }
                }

                if (std::exchange(quit_requested_, false)) {
                    // something requested that the application quits, so propagate this upwards
                    return AppMainLoopStatus::quit_requested();
                }

                if (next_widget_) {
                    // something requested a new top-level widget, so perform the transition
                    transition_to_next_top_level_widget();
                }
            }
        }

        // update clocks
        {
            const auto counter = SDL_GetPerformanceCounter();
            const Uint64 delta_ticks = counter - perf_counter_;

            perf_counter_ = counter;
            frame_start_time_ = convert_perf_counter_to_appclock(counter, perf_counter_frequency_);
            time_since_last_frame_ = convert_perf_ticks_to_appclock_duration(delta_ticks, perf_counter_frequency_);
        }

        // "tick" the widget
        {
            OSC_PERF("App/on_tick");
            current_widget_->on_tick();
        }

        if (std::exchange(quit_requested_, false)) {
            // something requested that the application quits, so propagate this upwards
            return AppMainLoopStatus::quit_requested();
        }

        if (next_widget_) {
            // something requested a new top-level widget, so perform the transition
            transition_to_next_top_level_widget();
            return AppMainLoopStatus::ok();
        }

        // "draw" the top-level widget into the main window framebuffer
        {
            OSC_PERF("App/on_draw");
            current_widget_->on_draw();
        }

        // "present" the framebuffer to the user (can block on VSYNC)
        {
            OSC_PERF("App/swap_buffers");
            graphics_context_.swap_buffers(*main_window_);
        }

        // handle annotated screenshot requests (if any)
        handle_screenshot_requests_for_this_frame();

        // care: only update the frame counter here because the above methods
        // and checks depend on it being consistent throughout a single crank
        // of the application loop
        ++frame_counter_;

        if (std::exchange(quit_requested_, false)) {
            // something requested that the application quits, so propagate this upwards
            return AppMainLoopStatus::quit_requested();
        }

        if (next_widget_) {
            // something requested a new top-level widget, so perform the transition
            transition_to_next_top_level_widget();
        }

        return AppMainLoopStatus::ok();
    }

    void teardown_main_loop()
    {
        if (current_widget_) {
            current_widget_->on_unmount();
            current_widget_.reset();
        }
        next_widget_.reset();

        main_window_annotations_this_frame_.clear();
        main_window_screenshot_requests_.clear();
    }

    void post_event(Widget& receiver, std::unique_ptr<Event> event)
    {
        SDL_Event e{};
        e.type = SDL_EVENT_USER;
        e.user.data1 = &receiver;
        e.user.data2 = event.release();
        SDL_PushEvent(&e);
    }

    bool notify(Widget& receiver, Event& event)
    {
        Widget* current = &receiver;
        do {
            if (current->on_event(event)) {
                return true;
            }
            current = current->parent();
        }
        while (current != nullptr and event.propagates());

        return false;
    }

    void show(std::unique_ptr<Widget> widget)
    {
        setup_main_loop(std::move(widget));

        // ensure `teardown_main_loop` is called - even if there's an exception
        const ScopeExit scope_guard{[this]() { teardown_main_loop(); }};

        while (do_main_loop_step()) {
            ;  // keep ticking the loop until it's not ok
        }
    }

    void request_transition(std::unique_ptr<Widget> widget)
    {
        next_widget_ = std::move(widget);
    }

    void request_quit()
    {
        quit_requested_ = true;
    }

    void request_invoke_on_main_thread(std::function<void()> callback)
    {
        SDL_Event e{};
        e.type = SDL_EVENT_USER;
        e.user.data1 = nullptr;
        e.user.data2 = std::make_unique<AppMarshalledCallbackEvent>(std::move(callback)).release();
        SDL_PushEvent(&e);  // Push the event onto the main thread's event queue (i.e. marshal it).
    }

    std::optional<std::filesystem::path> prompt_initial_directory_to_show_fallback()
    {
        return initial_directory_to_show_fallback_;
    }

    void set_prompt_initial_directory_to_show_fallback(const std::filesystem::path& p)
    {
        initial_directory_to_show_fallback_ = p;
    }

    void set_prompt_initial_directory_to_show_fallback(std::nullopt_t)
    {
        initial_directory_to_show_fallback_.reset();
    }

    void prompt_user_to_select_file_async(
        std::function<void(FileDialogResponse&&)> callback,
        std::span<const FileDialogFilter> filters,
        std::optional<std::filesystem::path> initial_directory_to_show,
        bool allow_many)
    {
        // Setup `SDL_ShowOpenFileDialog` arguments.
        auto dialog_callback_state = std::make_unique<SDL3DialogCallbackState>(std::move(callback), filters);
        const SDL_DialogFileFilter* sdl3_filters_ptr = dialog_callback_state->sdl3_filters.data();
        const auto sdl3_num_filters = static_cast<int>(dialog_callback_state->sdl3_filters.size());
        std::string default_location;
        if (initial_directory_to_show) {
            default_location = initial_directory_to_show->string();
        }
        else if (const auto fallback = prompt_initial_directory_to_show_fallback()) {
            default_location = fallback->string();
        }

        // Call into SDL3's dialog implementation.
        SDL_ShowOpenFileDialog(
            SDL3DialogCallbackState::sdl3_compatible_callback,
            dialog_callback_state.release(),
            main_window_.get(),  // make it modal in the main window
            sdl3_filters_ptr,
            sdl3_num_filters,
            default_location.empty() ? nullptr : default_location.c_str(),
            allow_many
        );

        // Ensure the UI immediately pumps the event queue etc. so that there isn't
        // a delay between the request and when the user sees the dialog.
        request_redraw();
    }

    void prompt_user_to_select_directory_async(
        std::function<void(FileDialogResponse&&)> callback,
        std::optional<std::filesystem::path> initial_directory_to_show,
        bool allow_many)
    {
        // Setup `SDL_ShowOpenFolderDialog` arguments.
        auto dialog_callback_state = std::make_unique<SDL3DialogCallbackState>(std::move(callback));
        std::string default_location;
        if (initial_directory_to_show) {
            default_location = initial_directory_to_show->string();
        }
        else if (const auto fallback = prompt_initial_directory_to_show_fallback()) {
            default_location = fallback->string();
        }

        // Call into SDL3's dialog implementation.
        SDL_ShowOpenFolderDialog(
            SDL3DialogCallbackState::sdl3_compatible_callback,
            dialog_callback_state.release(),
            main_window_.get(),
            default_location.empty() ? nullptr : default_location.c_str(),
            allow_many
        );
    }

    void prompt_user_to_save_file_async(
        std::function<void(FileDialogResponse&&)> callback,
        std::span<const FileDialogFilter> filters,
        std::optional<std::filesystem::path> initial_directory_to_show)
    {
        // Setup `SDL_ShowSaveFileDialog` arguments.
        auto sdl3_callback_state = std::make_unique<SDL3DialogCallbackState>(std::move(callback), filters);
        const SDL_DialogFileFilter* sdl3_filters_ptr = sdl3_callback_state->sdl3_filters.data();
        const auto sdl3_num_filters = static_cast<int>(sdl3_callback_state->sdl3_filters.size());
        std::string default_location;
        if (initial_directory_to_show) {
            default_location = initial_directory_to_show->string();
        }
        else if (const auto fallback = prompt_initial_directory_to_show_fallback()) {
            default_location = fallback->string();
        }

        // Call into SDL3's dialog implementation.
        SDL_ShowSaveFileDialog(
            SDL3DialogCallbackState::sdl3_compatible_callback,
            sdl3_callback_state.release(),
            main_window_.get(),  // make it modal in the main window
            sdl3_filters_ptr,
            sdl3_num_filters,
            default_location.empty() ? nullptr : default_location.c_str()
        );

        // Ensure the UI immediately pumps the event queue etc. so that there isn't
        // a delay between the request and when the user sees the dialog.
        request_redraw();
    }

    void prompt_user_to_save_file_with_extension_async(
        std::function<void(std::optional<std::filesystem::path>)> callback,
        std::optional<std::string_view> maybe_extension,
        std::optional<std::filesystem::path> initial_directory_to_show)
    {
        auto inner_callback = [caller_callback = std::move(callback), maybe_extension](FileDialogResponse&& response)  // NOLINT(cppcoreguidelines-rvalue-reference-param-not-moved)
        {
            if (response.has_error() or response.size() > 1) {
                return;  // Error, or the user somehow selected >1 file.
            }
            if (response.empty()) {
                caller_callback(std::nullopt);
                return;  // The user cancelled out of the dialog.
            }

            std::filesystem::path path = response.front();
            if (maybe_extension) {
                // ensure that the user-selected path is tested against '.$EXTENSION' (#771)
                //
                // the caller only provides the extension without the dot but the user may have
                // manually written a string that is suffixed with the dot-less version of the
                // extension (e.g. "somecsv")
                std::stringstream full_extension;
                full_extension << "." << *maybe_extension;
                if (not path.string().ends_with(full_extension.str())) {
                    path += full_extension.str();
                }
            }

            caller_callback(std::move(path));
        };
        std::vector<FileDialogFilter> filters;
        filters.reserve(2);  // Upper bound
        if (maybe_extension) {
            std::stringstream filter;
            filter << "*." << *maybe_extension;
            std::string filter_string{std::move(filter).str()};
            std::stringstream name;
            name << "Permitted File (" << filter_string << ')';
            filters.emplace_back(std::move(name).str(), *maybe_extension);
        }
        filters.push_back(FileDialogFilter::all_files());

        prompt_user_to_save_file_async(
            std::move(inner_callback),
            std::move(filters),
            std::move(initial_directory_to_show)
        );
    }

    WindowID main_window_id() const
    {
        return WindowID{main_window_.get()};
    }

    Vec2 main_window_dimensions() const
    {
        return main_window_pixel_dimensions() / main_window_device_pixel_ratio();
    }

    void try_async_set_main_window_dimensions(Vec2 new_dims)
    {
        // mirror `SDL_GetWindowSize` by figuring out the scale factor
        // difference between what the caller provides (virtual coords,
        // as scaled by us) and what `SDL_GetWindowSize` provides (unknown
        // coordinate system).

        Vec2i sdl_size;
        SDL_GetWindowSize(main_window_.get(), &sdl_size.x, &sdl_size.y);
        const Vec2 ratio = new_dims/main_window_dimensions();
        const Vec2i scaled_dims(ratio * Vec2{sdl_size});
        SDL_SetWindowSize(main_window_.get(), scaled_dims.x, scaled_dims.y);
    }

    Vec2 main_window_pixel_dimensions() const
    {
        int w = 0;
        int h = 0;
        SDL_GetWindowSizeInPixels(main_window_.get(), &w, &h);
        return Vec2{static_cast<float>(w), static_cast<float>(h)};
    }

    float main_window_device_pixel_ratio() const
    {
        return SDL_GetWindowDisplayScale(main_window_.get());
    }

    float highest_device_pixel_ratio() const
    {
        int displays = 0;
        SDL_DisplayID* display_list_head = SDL_GetDisplays(&displays);
        ScopeExit list_destructor{[display_list_head] { SDL_free(display_list_head); }};

        std::optional<float> rv;
        for (SDL_DisplayID* it = display_list_head; displays > 0; ++it, --displays) {
            rv = std::max(rv, std::optional{SDL_GetDisplayContentScale(*it)});
        }
        return rv.value_or(1.0f);
    }

    float os_to_main_window_device_independent_ratio() const
    {
        // i.e. scale the event by multiplying it by the pixel density (yielding a
        // pixel-based event value) and then dividing it by the suggested window
        // display scale (yielding a device-independent pixel value).
        return SDL_GetWindowPixelDensity(main_window_.get()) / SDL_GetWindowDisplayScale(main_window_.get());
    }

    bool is_main_window_minimized() const
    {
        return (SDL_GetWindowFlags(main_window_.get()) & SDL_WINDOW_MINIMIZED) != 0u;
    }

    void push_cursor_override(const Cursor& cursor)
    {
        cursor_handler_.push_cursor_override(cursor);
    }

    void pop_cursor_override()
    {
        cursor_handler_.pop_cursor_override();
    }

    void enable_main_window_grab()
    {
        SDL_SetWindowMouseGrab(main_window_.get(), true);
    }

    WindowID get_keyboard_focus() const
    {
        return WindowID{SDL_GetKeyboardFocus()};
    }

    void disable_main_window_grab()
    {
        SDL_SetWindowMouseGrab(main_window_.get(), false);
    }

    std::optional<Vec2> mouse_pos_in_main_window() const
    {
        if (SDL_GetMouseFocus() != main_window_.get()) {
            return std::nullopt;  // main window is unfocused
        }

        // SDL returns position of the mouse relative to the top-left corner
        // of the window in OS units
        Vec2 p;
        SDL_GetMouseState(&p.x, &p.y);

        // scale OS units to device-independent pixels
        p *= os_to_main_window_device_independent_ratio();

        // transform from left-handed origin-in-top-left coordinate system
        // to screen space
        p.y = main_window_dimensions().y - p.y;

        return p;
    }

    bool has_input_focus(WindowID window_id) const
    {
        return (SDL_GetWindowFlags(std::bit_cast<SDL_Window*>(to<void*>(window_id))) & SDL_WINDOW_INPUT_FOCUS) != 0;
    }

    void set_main_window_unicode_input_rect(const Rect& screen_rect)
    {
        // Convert to SDL3 units and ensure it's in the left-handed origin-is-top-left
        // coordinate system that SDL3 wants, then convert it into an `SDL_Rect`
        const SDL_Rect r = to<SDL_Rect>(
            screen_rect
            .with_flipped_y(main_window_dimensions().y)
            .with_origin_and_dimensions_scaled_by(1.0f/os_to_main_window_device_independent_ratio())
        );

        SDL_SetTextInputArea(main_window_.get(), &r, 0);
    }

    void start_text_input(WindowID window_id)
    {
        SDL_StartTextInput(std::bit_cast<SDL_Window*>(to<void*>(window_id)));
    }

    void stop_text_input(WindowID window_id)
    {
        SDL_StopTextInput(std::bit_cast<SDL_Window*>(to<void*>(window_id)));
    }

    void set_show_cursor(bool v)
    {
        if (v) {
            SDL_ShowCursor();
        }
        else {
            SDL_HideCursor();
        }
        SDL_SetWindowMouseGrab(main_window_.get(), not v);
    }

    void make_main_window_fullscreen()
    {
        SDL_SetWindowFullscreenMode(main_window_.get(), nullptr);
        SDL_SetWindowFullscreen(main_window_.get(), true);
    }

    void make_main_window_windowed()
    {
        SDL_SetWindowFullscreen(main_window_.get(), false);
    }

    AntiAliasingLevel anti_aliasing_level() const
    {
        return antialiasing_level_;
    }

    void set_anti_aliasing_level(AntiAliasingLevel s)
    {
        antialiasing_level_ = clamp(s, AntiAliasingLevel{1}, max_anti_aliasing_level());
    }

    AntiAliasingLevel max_anti_aliasing_level() const
    {
        return graphics_context_.max_antialiasing_level();
    }

    bool is_in_debug_mode() const
    {
        return graphics_context_.is_in_debug_mode();
    }

    void set_debug_mode(bool v)
    {
        graphics_context_.set_debug_mode(v);
    }

    bool is_vsync_enabled() const
    {
        return graphics_context_.is_vsync_enabled();
    }

    void set_vsync_enabled(bool v)
    {
        graphics_context_.set_vsync_enabled(v);
    }

    void add_main_window_frame_annotation(std::string_view label, const Rect& screen_rect)
    {
        main_window_annotations_this_frame_.emplace_back(std::string{label}, screen_rect);
    }

    std::future<Screenshot> request_screenshot_of_main_window()
    {
        AnnotatedScreenshotRequest& req = main_window_screenshot_requests_.emplace_back(frame_counter_, request_screenshot_texture());
        return req.result_promise.get_future();
    }

    std::string graphics_backend_vendor_string() const
    {
        return graphics_context_.backend_vendor_string();
    }

    std::string graphics_backend_renderer_string() const
    {
        return graphics_context_.backend_renderer_string();
    }

    std::string graphics_backend_version_string() const
    {
        return graphics_context_.backend_version_string();
    }

    std::string graphics_backend_shading_language_version_string() const
    {
        return graphics_context_.backend_shading_language_version_string();
    }

    size_t num_frames_drawn() const
    {
        return frame_counter_;
    }

    AppClock::time_point startup_time() const
    {
        return startup_time_;
    }

    AppClock::duration frame_delta_since_startup() const
    {
        return frame_start_time_ - startup_time_;
    }

    AppClock::time_point frame_start_time() const
    {
        return frame_start_time_;
    }

    AppClock::duration frame_delta_since_last_frame() const
    {
        return time_since_last_frame_;
    }

    bool is_main_loop_waiting() const
    {
        return is_in_wait_mode_;
    }

    void set_main_loop_waiting(bool v)
    {
        is_in_wait_mode_ = v;
        request_redraw();
    }

    void make_main_loop_waiting()
    {
        set_main_loop_waiting(true);
    }

    void make_main_loop_polling()
    {
        set_main_loop_waiting(false);
    }

    void request_redraw()
    {
        SDL_Event e{};
        e.type = SDL_EVENT_USER;
        num_frames_to_poll_ += 2;  // immediate rendering can require rendering 2 frames before it shows something
        SDL_PushEvent(&e);
    }

    void clear_main_window(const Color& color)
    {
        graphics_context_.clear_main_window(color);
    }

    void set_main_window_subtitle(std::string_view subtitle)
    {
        auto title_lock = main_window_subtitle_.lock();

        if (subtitle == *title_lock) {
            return;
        }

        *title_lock = subtitle;

        const std::string new_title = subtitle.empty() ?
            std::string{metadata_.human_readable_application_name()} :
            (std::string{subtitle} + " - " + metadata_.human_readable_application_name());

        SDL_SetWindowTitle(main_window_.get(), new_title.c_str());
    }

    void unset_main_window_subtitle()
    {
        set_main_window_subtitle({});
    }

    const AppSettings& get_config() const { return config_; }

    AppSettings& upd_settings() { return config_; }

    ResourceLoader& upd_resource_loader()
    {
        return resource_loader_;
    }

    std::filesystem::path get_resource_filepath(const ResourcePath& rp) const
    {
        return std::filesystem::weakly_canonical(resources_dir_ / rp.string());
    }

    std::string slurp_resource(const ResourcePath& rp)
    {
        return resource_loader_.slurp(rp);
    }

    ResourceStream go_load_resource(const ResourcePath& rp)
    {
        return resource_loader_.open(rp);
    }

    std::shared_ptr<void> upd_singleton(
        const std::type_info& type_info,
        const std::function<std::shared_ptr<void>()>& singleton_constructor)
    {
        auto lock = singletons_.lock();
        const auto [it, inserted] = lock->try_emplace(TypeInfoReference{type_info}, nullptr);
        if (inserted) {
            it->second = singleton_constructor();
        }
        return it->second;
    }

    GraphicsContext& upd_graphics_context() { return graphics_context_; }

private:
    bool is_window_focused() const
    {
        return (SDL_GetWindowFlags(main_window_.get()) & SDL_WINDOW_INPUT_FOCUS) != 0u;
    }

    std::future<Texture2D> request_screenshot_texture()
    {
        return graphics_context_.request_screenshot();
    }

    // transitions from the current top-level widget to the next top-level
    // widget (if available)
    void transition_to_next_top_level_widget()
    {
        if (not next_widget_) {
            return;
        }

        if (current_widget_) {
            log_info("unmounting widget '%s'", current_widget_->name().c_str());

            try {
                current_widget_->on_unmount();
            }
            catch (const std::exception& ex) {
                log_error("error unmounting widget '%s': %s", current_widget_->name().c_str(), ex.what());
                current_widget_.reset();
                throw;
            }

            current_widget_.reset();
        }

        current_widget_ = std::move(next_widget_);

        // the next top-level widget might need to draw a couple of frames
        // to "warm up" (e.g. because it's using an immediate ui)
        num_frames_to_poll_ = 2;

        log_info("mounting widget '%s'", current_widget_->name().c_str());
        current_widget_->on_mount();
    }

    // tries to handle any active (asynchronous) screenshot requests
    void handle_screenshot_requests_for_this_frame()
    {
        // save this frame's annotations into the requests, if necessary
        for (AnnotatedScreenshotRequest& req : main_window_screenshot_requests_) {
            if (req.frame_requested == frame_counter_) {
                req.annotations = main_window_annotations_this_frame_;
            }
        }
        main_window_annotations_this_frame_.clear();  // this frame's annotations are now saved (if necessary)

        // complete any requests for which screenshot data has arrived
        for (AnnotatedScreenshotRequest& req : main_window_screenshot_requests_) {

            if (req.underlying_future.valid() and
                req.underlying_future.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {

                // screenshot is ready: create an annotated screenshot and send it to
                // the caller
                req.result_promise.set_value(Screenshot{req.underlying_future.get(), std::move(req.annotations)});
            }
        }

        // gc any invalid (i.e. handled) requests
        std::erase_if(
            main_window_screenshot_requests_,
            [](const AnnotatedScreenshotRequest& request)
            {
                return not request.underlying_future.valid();
            }
        );
    }

    // immutable application metadata (can be provided at runtime via ctor)
    AppMetadata metadata_;

    // top-level application configuration
    AppSettings config_{
        metadata_.organization_name(),
        metadata_.application_name(),
        metadata_.config_filename(),
    };

    // initialization-time resources dir (so that it doesn't have to be fetched
    // from the settings over-and-over)
    std::filesystem::path resources_dir_ = get_resource_dir_from_settings(config_);

    // path to the directory that the application's executable is contained within
    std::filesystem::path executable_dir_ = get_current_exe_dir_and_log_it();

    // path to the write-able user data directory
    std::filesystem::path user_data_dir_ = get_current_user_dir_and_log_it(
        metadata_.organization_name(),
        metadata_.application_name()
    );

    // this is set by `set_initial_directory_to_show_fallback`, which is used to provide the
    // file dialog system with a hint of where the user probably expects the next dialog to
    // open
    std::optional<std::filesystem::path> initial_directory_to_show_fallback_;

    // ensures that the global application log is configured according to the
    // application's configuration file
    bool log_is_configured_ = configure_application_log(config_);

    // top-level runtime resource loader
    ResourceLoader resource_loader_ = make_resource_loader<FilesystemResourceLoader>(resources_dir_);

    // SDL context (windowing, video driver, etc.)
    sdl::Context sdl_context_{SDL_INIT_VIDEO};

    // SDL main application window
    sdl::Window main_window_ = create_main_app_window(config_, metadata_.human_readable_application_name());

    // cache for the current (caller-set) window subtitle
    SynchronizedValue<std::string> main_window_subtitle_;

    // 3D graphics context for the oscar graphics API
    GraphicsContext graphics_context_{*main_window_};

    // application-wide handler for the mouse cursor
    CursorHandler cursor_handler_;

    // get performance counter frequency (for the delta clocks)
    Uint64 perf_counter_frequency_ = SDL_GetPerformanceFrequency();

    // current performance counter value (recorded once per frame)
    Uint64 perf_counter_ = 0;

    // number of frames the application has drawn
    size_t frame_counter_ = 0;

    // when the application started up (set now)
    AppClock::time_point startup_time_ = convert_perf_counter_to_appclock(SDL_GetPerformanceCounter(), perf_counter_frequency_);

    // when the current frame started (set each frame)
    AppClock::time_point frame_start_time_ = startup_time_;

    // time since the frame before the current frame (set each frame)
    AppClock::duration time_since_last_frame_ = {};

    // application-wide cache of initialized singletons
    SynchronizedValue<ankerl::unordered_dense::map<TypeInfoReference, std::shared_ptr<void>>> singletons_;

    // how many antiAliasingLevel the implementation should actually use
    AntiAliasingLevel antialiasing_level_ = min(graphics_context_.max_antialiasing_level(), AntiAliasingLevel{4});

    // set to true if the application should quit
    bool quit_requested_ = false;

    // set to true if the main loop should pause on events
    //
    // CAREFUL: this makes the app event-driven
    bool is_in_wait_mode_ = false;

    // set >0 to force that `n` frames are polling-driven: even in waiting mode
    int32_t num_frames_to_poll_ = 0;

    // current top-level widget (if any)
    std::unique_ptr<Widget> current_widget_;

    // the *next* top-level widget (if any - usually via a request to transition to it)
    std::unique_ptr<Widget> next_widget_;

    // frame annotations made during this frame
    std::vector<ScreenshotAnnotation> main_window_annotations_this_frame_;

    // any active promises for an annotated frame
    std::vector<AnnotatedScreenshotRequest> main_window_screenshot_requests_;
};

App& osc::App::upd()
{
    OSC_ASSERT_ALWAYS(g_app_global && "App is not initialized: have you constructed a (singleton) instance of App?");
    return *g_app_global;
}

const App& osc::App::get()
{
    OSC_ASSERT_ALWAYS(g_app_global && "App is not initialized: have you constructed a (singleton) instance of App?");
    return *g_app_global;
}

const AppSettings& osc::App::settings()
{
    return get().get_config();
}

std::filesystem::path osc::App::resource_filepath(const ResourcePath& rp)
{
    return get().get_resource_filepath(rp);
}

std::string osc::App::slurp(const ResourcePath& rp)
{
    return upd().slurp_resource(rp);
}

ResourceStream osc::App::load_resource(const ResourcePath& rp)
{
    return upd().go_load_resource(rp);
}

ResourceLoader& osc::App::resource_loader()
{
    return upd().upd_resource_loader();
}

osc::App::App() :
    App{AppMetadata{}}
{}

osc::App::App(const AppMetadata& metadata)
{
    OSC_ASSERT(g_app_global == nullptr && "cannot instantiate multiple `App` instances at the same time");

    impl_ = std::make_unique<Impl>(metadata);
    g_app_global = this;
}

osc::App::~App() noexcept
{
    g_app_global = nullptr;
}

const AppMetadata& osc::App::metadata() const
{
    return impl_->metadata();
}

std::string osc::App::human_readable_name() const
{
    return impl_->human_readable_name();
}

std::string osc::App::application_name_with_version_and_buildid() const
{
    return impl_->application_name_with_version_and_buildid();
}

const std::filesystem::path& osc::App::executable_directory() const
{
    return impl_->executable_directory();
}

const std::filesystem::path& osc::App::user_data_directory() const
{
    return impl_->user_data_directory();
}

void osc::App::setup_main_loop(std::unique_ptr<Widget> widget)
{
    impl_->setup_main_loop(std::move(widget));
}

AppMainLoopStatus osc::App::do_main_loop_step()
{
    return impl_->do_main_loop_step();
}

void osc::App::teardown_main_loop()
{
    impl_->teardown_main_loop();
}

void osc::App::post_event(Widget& receiver, std::unique_ptr<Event> event)
{
    upd().impl_->post_event(receiver, std::move(event));
}

bool osc::App::notify(Widget& receiver, Event& event)
{
    return upd().impl_->notify(receiver, event);
}

void osc::App::show(std::unique_ptr<Widget> widget)
{
    impl_->show(std::move(widget));
}

void osc::App::request_transition(std::unique_ptr<Widget> widget)
{
    impl_->request_transition(std::move(widget));
}

void osc::App::request_quit()
{
    impl_->request_quit();
}

void osc::App::request_invoke_on_main_thread(std::function<void()> callback)
{
    impl_->request_invoke_on_main_thread(std::move(callback));
}

std::optional<std::filesystem::path> osc::App::prompt_initial_directory_to_show_fallback()
{
    return impl_->prompt_initial_directory_to_show_fallback();
}

void osc::App::set_prompt_initial_directory_to_show_fallback(const std::filesystem::path& p)
{
    impl_->set_prompt_initial_directory_to_show_fallback(p);
}

void osc::App::set_prompt_initial_directory_to_show_fallback(std::nullopt_t)
{
    impl_->set_prompt_initial_directory_to_show_fallback(std::nullopt);
}

void osc::App::prompt_user_to_select_file_async(
    std::function<void(FileDialogResponse&&)> callback,
    std::span<const FileDialogFilter> filters,
    std::optional<std::filesystem::path> initial_directory_to_show,
    bool allow_many)
{
    impl_->prompt_user_to_select_file_async(
        std::move(callback),
        filters,
        std::move(initial_directory_to_show),
        allow_many
    );
}

void osc::App::prompt_user_to_select_directory_async(
    std::function<void(FileDialogResponse&&)> callback,
    std::optional<std::filesystem::path> initial_directory_to_show,
    bool allow_many)
{
    impl_->prompt_user_to_select_directory_async(
        std::move(callback),
        std::move(initial_directory_to_show),
        allow_many
    );
}

void osc::App::prompt_user_to_save_file_async(
    std::function<void(FileDialogResponse&&)> callback,
    std::span<const FileDialogFilter> filters,
    std::optional<std::filesystem::path> initial_directory_to_show)
{
    impl_->prompt_user_to_save_file_async(
        std::move(callback),
        filters,
        std::move(initial_directory_to_show)
    );
}

void osc::App::prompt_user_to_save_file_with_extension_async(
    std::function<void(std::optional<std::filesystem::path>)> callback,
    std::optional<std::string_view> maybe_extension,
    std::optional<std::filesystem::path> initial_directory_to_show)
{
    impl_->prompt_user_to_save_file_with_extension_async(
        std::move(callback),
        maybe_extension,
        std::move(initial_directory_to_show)
    );
}

WindowID osc::App::main_window_id() const
{
    return impl_->main_window_id();
}

Vec2 osc::App::main_window_dimensions() const
{
    return impl_->main_window_dimensions();
}

void osc::App::try_async_set_main_window_dimensions(Vec2 new_dims)
{
    impl_->try_async_set_main_window_dimensions(new_dims);
}

Vec2 osc::App::main_window_pixel_dimensions() const
{
    return impl_->main_window_pixel_dimensions();
}

float osc::App::main_window_device_pixel_ratio() const
{
    return impl_->main_window_device_pixel_ratio();
}

float osc::App::highest_device_pixel_ratio() const
{
    return impl_->highest_device_pixel_ratio();
}

bool osc::App::is_main_window_minimized() const
{
    return impl_->is_main_window_minimized();
}

void osc::App::push_cursor_override(const Cursor& cursor)
{
    impl_->push_cursor_override(cursor);
}

void osc::App::pop_cursor_override()
{
    impl_->pop_cursor_override();
}

void osc::App::enable_main_window_grab()
{
    impl_->enable_main_window_grab();
}

WindowID osc::App::get_keyboard_focus() const
{
    return impl_->get_keyboard_focus();
}

void osc::App::disable_main_window_grab()
{
    impl_->disable_main_window_grab();
}

std::optional<Vec2> osc::App::mouse_pos_in_main_window() const
{
    return impl_->mouse_pos_in_main_window();
}

bool osc::App::has_input_focus(WindowID id) const
{
    return impl_->has_input_focus(id);
}

void osc::App::set_main_window_unicode_input_rect(const Rect& screen_rect)
{
    impl_->set_main_window_unicode_input_rect(screen_rect);
}

void osc::App::start_text_input(WindowID window_id)
{
    impl_->start_text_input(window_id);
}

void osc::App::stop_text_input(WindowID window_id)
{
    impl_->stop_text_input(window_id);
}

void osc::App::make_main_window_fullscreen()
{
    impl_->make_main_window_fullscreen();
}

void osc::App::make_main_window_windowed()
{
    impl_->make_main_window_windowed();
}

AntiAliasingLevel osc::App::anti_aliasing_level() const
{
    return impl_->anti_aliasing_level();
}

void osc::App::set_anti_aliasing_level(AntiAliasingLevel s)
{
    impl_->set_anti_aliasing_level(s);
}

AntiAliasingLevel osc::App::max_anti_aliasing_level() const
{
    return impl_->max_anti_aliasing_level();
}

bool osc::App::is_in_debug_mode() const
{
    return impl_->is_in_debug_mode();
}

void osc::App::set_debug_mode(bool v)
{
    impl_->set_debug_mode(v);
}

bool osc::App::is_vsync_enabled() const
{
    return impl_->is_vsync_enabled();
}

void osc::App::set_vsync_enabled(bool v)
{
    impl_->set_vsync_enabled(v);
}

void osc::App::add_main_window_frame_annotation(std::string_view label, const Rect& screen_rect)
{
    impl_->add_main_window_frame_annotation(label, screen_rect);
}

std::future<Screenshot> osc::App::request_screenshot_of_main_window()
{
    return impl_->request_screenshot_of_main_window();
}

std::string osc::App::graphics_backend_vendor_string() const
{
    return impl_->graphics_backend_vendor_string();
}

std::string osc::App::graphics_backend_renderer_string() const
{
    return impl_->graphics_backend_renderer_string();
}

std::string osc::App::graphics_backend_version_string() const
{
    return impl_->graphics_backend_version_string();
}

std::string osc::App::graphics_backend_shading_language_version_string() const
{
    return impl_->graphics_backend_shading_language_version_string();
}

size_t osc::App::num_frames_drawn() const
{
    return impl_->num_frames_drawn();
}

AppClock::time_point osc::App::startup_time() const
{
    return impl_->startup_time();
}

AppClock::duration osc::App::frame_delta_since_startup() const
{
    return impl_->frame_delta_since_startup();
}

AppClock::time_point osc::App::frame_start_time() const
{
    return impl_->frame_start_time();
}

AppClock::duration osc::App::frame_delta_since_last_frame() const
{
    return impl_->frame_delta_since_last_frame();
}

bool osc::App::is_main_loop_waiting() const
{
    return impl_->is_main_loop_waiting();
}

void osc::App::set_main_loop_waiting(bool v)
{
    impl_->set_main_loop_waiting(v);
}

void osc::App::make_main_loop_waiting()
{
    impl_->make_main_loop_waiting();
}

void osc::App::make_main_loop_polling()
{
    impl_->make_main_loop_polling();
}

void osc::App::request_redraw()
{
    impl_->request_redraw();
}

void osc::App::clear_main_window(const Color& color)
{
    impl_->clear_main_window(color);
}

void osc::App::set_main_window_subtitle(std::string_view subtitle)
{
    impl_->set_main_window_subtitle(subtitle);
}

void osc::App::unset_main_window_subtitle()
{
    impl_->unset_main_window_subtitle();
}

const AppSettings& osc::App::get_config() const
{
    return impl_->get_config();
}

AppSettings& osc::App::upd_settings()
{
    return impl_->upd_settings();
}

ResourceLoader& osc::App::upd_resource_loader()
{
    return impl_->upd_resource_loader();
}

std::filesystem::path osc::App::get_resource_filepath(const ResourcePath& rp) const
{
    return impl_->get_resource_filepath(rp);
}

std::string osc::App::slurp_resource(const ResourcePath& rp)
{
    return impl_->slurp_resource(rp);
}

ResourceStream osc::App::go_load_resource(const ResourcePath& rp)
{
    return impl_->go_load_resource(rp);
}

int osc::App::main_internal(const AppMetadata& metadata, const std::function<std::unique_ptr<Widget>()>& widget_ctor)
{
    // If running via EMSCRIPTEN, then the engine (usually, browser) is
    // responsible for calling into each step of the render loop.
#ifdef EMSCRIPTEN
    App* app = new App{metadata};
    app->setup_main_loop(widget_ctor());
    emscripten_set_main_loop_arg([](void* ptr) -> void
    {
        if (not static_cast<App*>(ptr)->do_main_loop_step()) {
            throw std::runtime_error{"exit"};
        }
    }, app, 0, EM_TRUE);
    return 0;
#else
    App app{metadata};
    app.show(widget_ctor());
    return 0;
#endif
}

std::shared_ptr<void> osc::App::upd_singleton(
    const std::type_info& type_info,
    const std::function<std::shared_ptr<void>()>& singleton_constructor)
{
    return impl_->upd_singleton(type_info, singleton_constructor);
}

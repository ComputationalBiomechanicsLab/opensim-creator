#include "Event.h"

#include <oscar/Platform/App.h>
#include <oscar/Platform/Log.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/Conversion.h>
#include <oscar/Utils/EnumHelpers.h>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>

#include <array>
#include <memory>
#include <utility>

using namespace osc;

template<>
struct osc::Converter<Uint16, KeyModifier> final {
    KeyModifier operator()(Uint16 mod) const
    {
        KeyModifier rv = KeyModifier::None;
        for (const auto& [sdl_modifier, osc_modifier] : c_mappings_) {
            if (mod & sdl_modifier) {
                rv |= osc_modifier;
            }
        }
        return rv;
    }
private:
    using Mapping = std::pair<SDL_Keymod, KeyModifier>;
    static constexpr std::array<Mapping, num_flags<KeyModifier>()> c_mappings_ = std::to_array<Mapping>({
        {SDL_KMOD_LSHIFT, KeyModifier::LeftShift},
        {SDL_KMOD_RSHIFT, KeyModifier::RightShift},
        {SDL_KMOD_LCTRL, KeyModifier::LeftCtrl},
        {SDL_KMOD_RCTRL, KeyModifier::RightCtrl},
        {SDL_KMOD_LALT, KeyModifier::LeftAlt},
        {SDL_KMOD_RALT, KeyModifier::RightAlt},
        {SDL_KMOD_LGUI, KeyModifier::LeftGui},
        {SDL_KMOD_RGUI, KeyModifier::RightGui},
    });
};

template<>
struct osc::Converter<SDL_Keycode, Key> final {
    Key operator()(SDL_Keycode code) const
    {
        static_assert(num_options<Key>() == 120);

        switch (code) {
        case SDLK_TAB: return Key::Tab;
        case SDLK_LEFT: return Key::LeftArrow;
        case SDLK_RIGHT: return Key::RightArrow;
        case SDLK_UP: return Key::UpArrow;
        case SDLK_DOWN: return Key::DownArrow;
        case SDLK_PAGEUP: return Key::PageUp;
        case SDLK_PAGEDOWN: return Key::PageDown;
        case SDLK_HOME: return Key::Home;
        case SDLK_END: return Key::End;
        case SDLK_INSERT: return Key::Insert;
        case SDLK_DELETE: return Key::Delete;
        case SDLK_BACKSPACE: return Key::Backspace;
        case SDLK_SPACE: return Key::Space;
        case SDLK_RETURN: return Key::Return;
        case SDLK_ESCAPE: return Key::Escape;
        case SDLK_APOSTROPHE: return Key::Apostrophe;
        case SDLK_COMMA: return Key::Comma;
        case SDLK_MINUS: return Key::Minus;
        case SDLK_PERIOD: return Key::Period;
        case SDLK_SLASH: return Key::Slash;
        case SDLK_SEMICOLON: return Key::Semicolon;
        case SDLK_EQUALS: return Key::Equals;
        case SDLK_LEFTBRACKET: return Key::LeftBracket;
        case SDLK_BACKSLASH: return Key::Backslash;
        case SDLK_RIGHTBRACKET: return Key::RightBracket;
        case SDLK_GRAVE: return Key::Grave;
        case SDLK_CAPSLOCK: return Key::CapsLock;
        case SDLK_SCROLLLOCK: return Key::ScrollLock;
        case SDLK_NUMLOCKCLEAR: return Key::NumLockClear;
        case SDLK_PRINTSCREEN: return Key::PrintScreen;
        case SDLK_PAUSE: return Key::Pause;
        case SDLK_KP_0: return Key::Keypad0;
        case SDLK_KP_1: return Key::Keypad1;
        case SDLK_KP_2: return Key::Keypad2;
        case SDLK_KP_3: return Key::Keypad3;
        case SDLK_KP_4: return Key::Keypad4;
        case SDLK_KP_5: return Key::Keypad5;
        case SDLK_KP_6: return Key::Keypad6;
        case SDLK_KP_7: return Key::Keypad7;
        case SDLK_KP_8: return Key::Keypad8;
        case SDLK_KP_9: return Key::Keypad9;
        case SDLK_KP_PERIOD: return Key::KeypadPeriod;
        case SDLK_KP_DIVIDE: return Key::KeypadDivide;
        case SDLK_KP_MULTIPLY: return Key::KeypadMultiply;
        case SDLK_KP_MINUS: return Key::KeypadMinus;
        case SDLK_KP_PLUS: return Key::KeypadPlus;
        case SDLK_KP_ENTER: return Key::KeypadEnter;
        case SDLK_KP_EQUALS: return Key::KeypadEquals;
        case SDLK_LCTRL: return Key::LeftCtrl;
        case SDLK_LSHIFT: return Key::LeftShift;
        case SDLK_LALT: return Key::LeftAlt;
        case SDLK_LGUI: return Key::LeftGui;
        case SDLK_RCTRL: return Key::RightCtrl;
        case SDLK_RSHIFT: return Key::RightShift;
        case SDLK_RALT: return Key::RightAlt;
        case SDLK_RGUI: return Key::RightGui;
        case SDLK_APPLICATION: return Key::Application;
        case SDLK_0: return Key::_0;
        case SDLK_1: return Key::_1;
        case SDLK_2: return Key::_2;
        case SDLK_3: return Key::_3;
        case SDLK_4: return Key::_4;
        case SDLK_5: return Key::_5;
        case SDLK_6: return Key::_6;
        case SDLK_7: return Key::_7;
        case SDLK_8: return Key::_8;
        case SDLK_9: return Key::_9;
        case SDLK_A: return Key::A;
        case SDLK_B: return Key::B;
        case SDLK_C: return Key::C;
        case SDLK_D: return Key::D;
        case SDLK_E: return Key::E;
        case SDLK_F: return Key::F;
        case SDLK_G: return Key::G;
        case SDLK_H: return Key::H;
        case SDLK_I: return Key::I;
        case SDLK_J: return Key::J;
        case SDLK_K: return Key::K;
        case SDLK_L: return Key::L;
        case SDLK_M: return Key::M;
        case SDLK_N: return Key::N;
        case SDLK_O: return Key::O;
        case SDLK_P: return Key::P;
        case SDLK_Q: return Key::Q;
        case SDLK_R: return Key::R;
        case SDLK_S: return Key::S;
        case SDLK_T: return Key::T;
        case SDLK_U: return Key::U;
        case SDLK_V: return Key::V;
        case SDLK_W: return Key::W;
        case SDLK_X: return Key::X;
        case SDLK_Y: return Key::Y;
        case SDLK_Z: return Key::Z;
        case SDLK_F1: return Key::F1;
        case SDLK_F2: return Key::F2;
        case SDLK_F3: return Key::F3;
        case SDLK_F4: return Key::F4;
        case SDLK_F5: return Key::F5;
        case SDLK_F6: return Key::F6;
        case SDLK_F7: return Key::F7;
        case SDLK_F8: return Key::F8;
        case SDLK_F9: return Key::F9;
        case SDLK_F10: return Key::F10;
        case SDLK_F11: return Key::F11;
        case SDLK_F12: return Key::F12;
        case SDLK_F13: return Key::F13;
        case SDLK_F14: return Key::F14;
        case SDLK_F15: return Key::F15;
        case SDLK_F16: return Key::F16;
        case SDLK_F17: return Key::F17;
        case SDLK_F18: return Key::F18;
        case SDLK_F19: return Key::F19;
        case SDLK_F20: return Key::F20;
        case SDLK_F21: return Key::F21;
        case SDLK_F22: return Key::F22;
        case SDLK_F23: return Key::F23;
        case SDLK_F24: return Key::F24;
        case SDLK_AC_BACK: return Key::AppBack;
        case SDLK_AC_FORWARD: return Key::AppForward;
        default: return Key::Unknown;
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

osc::DropFileEvent::DropFileEvent(const SDL_Event& e) :
    Event{EventType::DropFile},
    path_{e.drop.data}
{
    OSC_ASSERT(e.type == SDL_EVENT_DROP_FILE);
    OSC_ASSERT(e.drop.data != nullptr);
}

osc::KeyEvent::KeyEvent(const SDL_Event& e) :
    Event{e.type == SDL_EVENT_KEY_UP ? EventType::KeyUp : EventType::KeyDown},
    modifier_{to<KeyModifier>(e.key.mod)},
    key_{to<Key>(e.key.key)}
{
    OSC_ASSERT(e.type == SDL_EVENT_KEY_DOWN or e.type == SDL_EVENT_KEY_UP);
}

osc::QuitEvent::QuitEvent(const SDL_Event& e) :
    Event{EventType::Quit}
{
    OSC_ASSERT(e.type == SDL_EVENT_QUIT);
}

osc::TextInputEvent::TextInputEvent(const SDL_Event& e) :
    Event{EventType::TextInput},
    utf8_text_{static_cast<const char*>(e.text.text)}
{
    OSC_ASSERT(e.type == SDL_EVENT_TEXT_INPUT);
}

osc::DisplayStateChangeEvent::DisplayStateChangeEvent(const SDL_Event&) :
    Event{EventType::DisplayStateChange}
{}

osc::WindowEvent::WindowEvent(const SDL_Event& e) :
    Event{EventType::Window}
{
    static_assert(num_options<WindowEventType>() == 8);
    OSC_ASSERT(SDL_EVENT_WINDOW_FIRST <= e.type and e.type <= SDL_EVENT_WINDOW_LAST);

    switch (e.type) {
    case SDL_EVENT_WINDOW_MOUSE_ENTER:           type_ = WindowEventType::GainedMouseFocus;          break;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:           type_ = WindowEventType::LostMouseFocus;            break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:          type_ = WindowEventType::GainedKeyboardFocus;       break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:            type_ = WindowEventType::LostKeyboardFocus;         break;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:       type_ = WindowEventType::WindowClosed;              break;
    case SDL_EVENT_WINDOW_MOVED:                 type_ = WindowEventType::WindowMoved;               break;
    case SDL_EVENT_WINDOW_RESIZED:               type_ = WindowEventType::WindowResized;             break;
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: type_ = WindowEventType::WindowDisplayScaleChanged; break;
    }
    window_ = SDL_GetWindowFromID(e.window.windowID);
    window_id_ = e.window.windowID;
}

osc::MouseEvent::MouseEvent(const SDL_Event& e) :
    Event{e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? EventType::MouseButtonDown : (e.type == SDL_EVENT_MOUSE_BUTTON_UP ? EventType::MouseButtonUp : EventType::MouseMove)}
{
    switch (e.type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        input_source_ = e.button.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse;
        button_ = to<MouseButton>(e.button.button);
        break;
    }
    case SDL_EVENT_MOUSE_MOTION: {
        // scales from SDL3 (events) to device-independent pixels
        const float sdl3_to_device_independent_ratio = App::get().os_to_main_window_device_independent_ratio();

        relative_delta_ = {static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel)};
        relative_delta_ *= sdl3_to_device_independent_ratio;
        position_in_window_ = {static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
        position_in_window_ *= sdl3_to_device_independent_ratio;
        input_source_ = e.motion.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse;
        break;
    }
    default: {
        throw std::runtime_error{"unknown mouse event type passed into an osc::MouseEvent"};
    }}
}

osc::MouseWheelEvent::MouseWheelEvent(const SDL_Event& e) :
    Event{EventType::MouseWheel},
    delta_{e.wheel.x, e.wheel.y},
    input_source_{e.wheel.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse}
{
    OSC_ASSERT(e.type == SDL_EVENT_MOUSE_WHEEL);
}

std::unique_ptr<Event> osc::try_parse_into_event(const SDL_Event& e)
{
    if (e.type == SDL_EVENT_DROP_FILE and e.drop.data) {
        return std::make_unique<DropFileEvent>(e);
    }
    else if (e.type == SDL_EVENT_KEY_DOWN or e.type == SDL_EVENT_KEY_UP) {
        return std::make_unique<KeyEvent>(e);
    }
    else if (e.type == SDL_EVENT_QUIT) {
        return std::make_unique<QuitEvent>(e);
    }
    else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN or e.type == SDL_EVENT_MOUSE_BUTTON_UP or e.type == SDL_EVENT_MOUSE_MOTION) {
        return std::make_unique<MouseEvent>(e);
    }
    else if (e.type == SDL_EVENT_MOUSE_WHEEL) {
        return std::make_unique<MouseWheelEvent>(e);
    }
    else if (e.type == SDL_EVENT_TEXT_INPUT) {
        return std::make_unique<TextInputEvent>(e);
    }
    else if (SDL_EVENT_DISPLAY_FIRST <= e.type and e.type <=  SDL_EVENT_DISPLAY_LAST) {
        return std::make_unique<DisplayStateChangeEvent>(e);
    }
    else if (SDL_EVENT_WINDOW_FIRST <= e.type and e.type <= SDL_EVENT_WINDOW_LAST) {
        return std::make_unique<WindowEvent>(e);
    }
    else {
        return nullptr;
    }
}

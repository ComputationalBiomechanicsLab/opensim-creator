#include "Event.h"

#include <oscar/Platform/Log.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/Conversion.h>
#include <oscar/Utils/EnumHelpers.h>

#include <SDL_events.h>

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
        {KMOD_LSHIFT, KeyModifier::LeftShift},
        {KMOD_RSHIFT, KeyModifier::RightShift},
        {KMOD_LCTRL, KeyModifier::LeftCtrl},
        {KMOD_RCTRL, KeyModifier::RightCtrl},
        {KMOD_LALT, KeyModifier::LeftAlt},
        {KMOD_RALT, KeyModifier::RightAlt},
        {KMOD_LGUI, KeyModifier::LeftGui},
        {KMOD_RGUI, KeyModifier::RightGui},
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
        case SDLK_QUOTE: return Key::Quote;
        case SDLK_COMMA: return Key::Comma;
        case SDLK_MINUS: return Key::Minus;
        case SDLK_PERIOD: return Key::Period;
        case SDLK_SLASH: return Key::Slash;
        case SDLK_SEMICOLON: return Key::Semicolon;
        case SDLK_EQUALS: return Key::Equals;
        case SDLK_LEFTBRACKET: return Key::LeftBracket;
        case SDLK_BACKSLASH: return Key::Backslash;
        case SDLK_RIGHTBRACKET: return Key::RightBracket;
        case SDLK_BACKQUOTE: return Key::Backquote;
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
        case SDLK_a: return Key::A;
        case SDLK_b: return Key::B;
        case SDLK_c: return Key::C;
        case SDLK_d: return Key::D;
        case SDLK_e: return Key::E;
        case SDLK_f: return Key::F;
        case SDLK_g: return Key::G;
        case SDLK_h: return Key::H;
        case SDLK_i: return Key::I;
        case SDLK_j: return Key::J;
        case SDLK_k: return Key::K;
        case SDLK_l: return Key::L;
        case SDLK_m: return Key::M;
        case SDLK_n: return Key::N;
        case SDLK_o: return Key::O;
        case SDLK_p: return Key::P;
        case SDLK_q: return Key::Q;
        case SDLK_r: return Key::R;
        case SDLK_s: return Key::S;
        case SDLK_t: return Key::T;
        case SDLK_u: return Key::U;
        case SDLK_v: return Key::V;
        case SDLK_w: return Key::W;
        case SDLK_x: return Key::X;
        case SDLK_y: return Key::Y;
        case SDLK_z: return Key::Z;
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
    Event{e, EventType::DropFile},
    path_{e.drop.file}
{
    OSC_ASSERT(e.type == SDL_DROPFILE);
    OSC_ASSERT(e.drop.file != nullptr);
}

osc::KeyEvent::KeyEvent(const SDL_Event& e) :
    Event{e, e.type == SDL_KEYUP ? EventType::KeyUp : EventType::KeyDown},
    modifier_{to<KeyModifier>(e.key.keysym.mod)},
    key_{to<Key>(e.key.keysym.sym)}
{
    OSC_ASSERT(e.type == SDL_KEYDOWN or e.type == SDL_KEYUP);
}

osc::QuitEvent::QuitEvent(const SDL_Event& e) :
    Event{e, EventType::Quit}
{
    OSC_ASSERT(e.type == SDL_QUIT);
}

osc::MouseEvent::MouseEvent(const SDL_Event& e) :
    Event{e, e.type == SDL_MOUSEBUTTONDOWN ? EventType::MouseButtonDown : (e.type == SDL_MOUSEBUTTONUP ? EventType::MouseButtonUp : EventType::MouseMove)}
{
    switch (e.type) {
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: {
        input_source_ = e.button.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse;
        button_ = to<MouseButton>(e.button.button);
        break;
    }
    case SDL_MOUSEMOTION: {
        relative_delta_ = {static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel)};
        position_in_window_ = {static_cast<float>(e.motion.x), static_cast<float>(e.motion.y)};
        input_source_ = e.motion.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse;
        break;
    }
    default: {
        throw std::runtime_error{"unknown mouse event type passed into an osc::MouseEvent"};
    }
    }
}

osc::MouseWheelEvent::MouseWheelEvent(const SDL_Event& e) :
    Event{e, EventType::MouseWheel},
    delta_{e.wheel.preciseX, e.wheel.preciseY},
    input_source_{e.wheel.which == SDL_TOUCH_MOUSEID ? MouseInputSource::TouchScreen : MouseInputSource::Mouse}
{
    OSC_ASSERT(e.type == SDL_MOUSEWHEEL);
}

std::unique_ptr<Event> osc::parse_into_event(const SDL_Event& e)
{
    if (e.type == SDL_DROPFILE and e.drop.file) {
        return std::make_unique<DropFileEvent>(e);
    }
    else if (e.type == SDL_KEYDOWN or e.type == SDL_KEYUP) {
        return std::make_unique<KeyEvent>(e);
    }
    else if (e.type == SDL_QUIT) {
        return std::make_unique<QuitEvent>(e);
    }
    else if (e.type == SDL_MOUSEBUTTONDOWN or e.type == SDL_MOUSEBUTTONUP or e.type == SDL_MOUSEMOTION) {
        return std::make_unique<MouseEvent>(e);
    }
    else if (e.type == SDL_MOUSEWHEEL) {
        return std::make_unique<MouseWheelEvent>(e);
    }
    else {
        return std::make_unique<RawEvent>(e);
    }
}

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
        static_assert(num_options<Key>() == 67);

        switch (code) {
        case SDLK_ESCAPE: return Key::Escape;
        case SDLK_RETURN: return Key::Enter;
        case SDLK_SPACE: return Key::Space;
        case SDLK_DELETE: return Key::Delete;
        case SDLK_TAB: return Key::Tab;
        case SDLK_BACKSPACE: return Key::Backspace;
        case SDLK_PAGEUP: return Key::PageUp;
        case SDLK_PAGEDOWN: return Key::PageDown;
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
        case SDLK_1: return Key::_1;
        case SDLK_2: return Key::_2;
        case SDLK_3: return Key::_3;
        case SDLK_4: return Key::_4;
        case SDLK_5: return Key::_5;
        case SDLK_6: return Key::_6;
        case SDLK_7: return Key::_7;
        case SDLK_8: return Key::_8;
        case SDLK_9: return Key::_9;
        case SDLK_0: return Key::_0;
        case SDLK_UP: return Key::UpArrow;
        case SDLK_DOWN: return Key::DownArrow;
        case SDLK_LEFT: return Key::LeftArrow;
        case SDLK_RIGHT: return Key::RightArrow;
        case SDLK_MINUS: return Key::Minus;
        case SDLK_EQUALS: return Key::Equal;
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
        default:     return Key::Unknown;
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
    Event{e, e.type == SDL_KEYUP ? EventType::KeyRelease : EventType::KeyPress},
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
    Event{e, e.type == SDL_MOUSEBUTTONDOWN ? EventType::MouseButtonPress : (e.type == SDL_MOUSEBUTTONUP ? EventType::MouseButtonRelease : EventType::MouseMove)},
    relative_delta_{static_cast<float>(e.motion.xrel), static_cast<float>(e.motion.yrel)}
{
    OSC_ASSERT(e.type == SDL_MOUSEBUTTONDOWN or e.type == SDL_MOUSEBUTTONUP or e.type == SDL_MOUSEMOTION);
}

osc::MouseWheelEvent::MouseWheelEvent(const SDL_Event& e) :
    Event{e, EventType::MouseWheel},
    delta_{e.wheel.preciseX, e.wheel.preciseY}
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

#include "screen.hpp"

#include "application.hpp"

#include <cassert>

void osmv::Screen::on_application_mount(Application* a) {
    assert(a != nullptr);
    parent = a;
}

osmv::Application& osmv::Screen::application() const noexcept {
    assert(parent != nullptr);
    return *parent;
}

void osmv::Screen::request_screen_transition(std::unique_ptr<Screen> new_screen) {
    parent->request_screen_transition(std::move(new_screen));
}

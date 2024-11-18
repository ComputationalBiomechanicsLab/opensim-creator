#include "Screen.h"

#include <oscar/Platform/ScreenPrivate.h>

#include <memory>
#include <utility>

using namespace osc;

osc::Screen::Screen() :
    Screen{std::make_unique<ScreenPrivate>(*this, nullptr, "DefaultScreen")}
{}
osc::Screen::Screen(std::unique_ptr<ScreenPrivate>&& ptr) :
    Widget{std::move(ptr)}
{}

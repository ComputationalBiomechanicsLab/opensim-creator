#pragma once

#include <nonstd/span.hpp>

#include <iosfwd>

namespace osc
{
    class SceneDecoration;
}

namespace osc
{
    void WriteDecorationsAsDAE(nonstd::span<SceneDecoration const>, std::ostream&);
}
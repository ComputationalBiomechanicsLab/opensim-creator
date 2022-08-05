#pragma once

#include <nonstd/span.hpp>

#include <iosfwd>

namespace osc
{
    class SceneDecorationNew;
}

namespace osc
{
    void WriteDecorationsAsDAE(nonstd::span<SceneDecorationNew const>, std::ostream&);
}
#pragma once

#include <nonstd/span.hpp>

#include <iosfwd>

namespace osc
{
    class BasicSceneElement;
}

namespace osc
{
    void WriteDecorationsAsDAE(nonstd::span<BasicSceneElement const>, std::ostream&);
}
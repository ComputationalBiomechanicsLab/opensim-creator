#pragma once

#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // which primitive geometry the mesh data represents
    enum class MeshTopography {
        Triangles = 0,
        Lines,
        TOTAL,
    };
    std::ostream& operator<<(std::ostream&, MeshTopography);
}
#pragma once

#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // which primitive geometry the mesh data represents
    enum class MeshTopology {
        Triangles = 0,
        Lines,
        NUM_OPTIONS,
    };

    std::ostream& operator<<(std::ostream&, MeshTopology);
}

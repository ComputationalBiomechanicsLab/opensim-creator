#pragma once

#include <cstdint>
#include <iosfwd>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // which primitive geometry the mesh data represents
    enum class MeshTopology : int32_t {
        Triangles = 0,
        Lines,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, MeshTopology);
}
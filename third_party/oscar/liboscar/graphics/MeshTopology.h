#pragma once

#include <iosfwd>

namespace osc
{
    // which primitive geometry the mesh data represents
    enum class MeshTopology {
        Triangles,
        Lines,
        NUM_OPTIONS,

        Default = Triangles,
    };

    std::ostream& operator<<(std::ostream&, MeshTopology);
}

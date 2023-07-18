#pragma once

#include <iosfwd>

namespace osc { class Mesh; }

namespace osc
{
    void WriteMeshAsStl(
        std::ostream&,
        Mesh const&
    );
}

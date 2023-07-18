#pragma once

#include <cstdint>
#include <iosfwd>

namespace osc { class Mesh; }

namespace osc
{
    using ObjWriterFlags = int32_t;
    enum ObjWriterFlags_ {
        ObjWriterFlags_None = 0,
        ObjWriterFlags_NoWriteNormals = 1<<0,
        ObjWriterFlags_Default = ObjWriterFlags_None,
    };

    void WriteMeshAsObj(
        std::ostream&,
        Mesh const&,
        ObjWriterFlags = ObjWriterFlags_Default
    );
}

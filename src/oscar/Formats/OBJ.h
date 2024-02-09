#pragma once

#include <oscar/Shims/Cpp23/utility.h>

#include <cstdint>
#include <ctime>
#include <iosfwd>
#include <string>
#include <string_view>

namespace osc { class Mesh; }

namespace osc
{
    enum class ObjWriterFlags : uint32_t {
        None           = 0,
        NoWriteNormals = 1u<<0u,

        Default = None,
    };

    constexpr bool operator&(ObjWriterFlags lhs, ObjWriterFlags rhs)
    {
        return cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs);
    }

    struct ObjMetadata final {
        explicit ObjMetadata(
            std::string_view authoringTool_
        );

        std::string authoringTool;
        std::tm creationTime;
    };

    void WriteMeshAsObj(
        std::ostream&,
        Mesh const&,
        ObjMetadata const&,
        ObjWriterFlags = ObjWriterFlags::Default
    );
}

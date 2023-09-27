#pragma once

#include <cstdint>
#include <ctime>
#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>

namespace osc { class Mesh; }

namespace osc
{
    enum class ObjWriterFlags : uint32_t {
        None           = 0,
        NoWriteNormals = 1u<<0u,

        Default = None,
    };

    constexpr bool operator&(ObjWriterFlags a, ObjWriterFlags b) noexcept
    {
        using T = std::underlying_type_t<ObjWriterFlags>;
        return static_cast<T>(a) & static_cast<T>(b);
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

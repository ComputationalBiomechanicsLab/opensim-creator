#pragma once

#include <oscar/Shims/Cpp23/utility.hpp>

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
        return osc::to_underlying(a) & osc::to_underlying(b);
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

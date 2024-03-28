#pragma once

#include <oscar/Shims/Cpp23/utility.h>

#include <ctime>
#include <iosfwd>
#include <string>
#include <string_view>

namespace osc { class Mesh; }

namespace osc
{
    enum class ObjWriterFlags {
        None           = 0,
        NoWriteNormals = 1<<0,

        Default = None,
    };

    constexpr bool operator&(ObjWriterFlags lhs, ObjWriterFlags rhs)
    {
        return cpp23::to_underlying(lhs) & cpp23::to_underlying(rhs);
    }

    struct ObjMetadata final {
        explicit ObjMetadata(
            std::string_view authoring_tool_
        );

        std::string authoring_tool;
        std::tm creation_time;
    };

    void write_as_obj(
        std::ostream&,
        const Mesh&,
        const ObjMetadata&,
        ObjWriterFlags = ObjWriterFlags::Default
    );
}

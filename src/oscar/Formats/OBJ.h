#pragma once

#include <oscar/Utils/Flags.h>

#include <ctime>
#include <iosfwd>
#include <string>
#include <string_view>

namespace osc { class Mesh; }

namespace osc
{
    enum class ObjWriterFlag {
        None           = 0,
        NoWriteNormals = 1<<0,

        Default = None,
    };
    using ObjWriterFlags = Flags<ObjWriterFlag>;

    struct ObjMetadata final {
        explicit ObjMetadata();
        explicit ObjMetadata(std::string_view authoring_tool_);

        std::string authoring_tool;
        std::tm creation_time;
    };

    void write_as_obj(
        std::ostream&,
        const Mesh&,
        const ObjMetadata& = ObjMetadata{},
        ObjWriterFlags = ObjWriterFlag::Default
    );
}

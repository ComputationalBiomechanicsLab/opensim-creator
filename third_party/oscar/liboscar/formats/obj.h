#pragma once

#include <liboscar/utils/flags.h>

#include <ctime>
#include <iosfwd>
#include <string>
#include <string_view>

namespace osc { class Mesh; }

namespace osc
{
    enum class OBJWriterFlag {
        None           = 0,
        NoWriteNormals = 1<<0,

        Default = None,
    };
    using OBJWriterFlags = Flags<OBJWriterFlag>;

    struct OBJMetadata final {
        explicit OBJMetadata();
        explicit OBJMetadata(std::string_view authoring_tool_);

        std::string authoring_tool;
        std::tm creation_time;
    };

    class OBJ final {
    public:
        static void write(
            std::ostream&,
            const Mesh&,
            const OBJMetadata& = OBJMetadata{},
            OBJWriterFlags = OBJWriterFlag::Default
        );
    };
}

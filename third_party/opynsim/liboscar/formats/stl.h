#pragma once

#include <ctime>
#include <iosfwd>
#include <string>
#include <string_view>

namespace osc { class Mesh; }

namespace osc
{
    struct STLMetadata final {

        explicit STLMetadata();

        explicit STLMetadata(
            std::string_view authoring_tool_
        );

        std::string authoring_tool;
        std::tm creation_time;
    };

    class STL final {
    public:
        static void write(std::ostream&, const Mesh&, const STLMetadata&);
    };
}

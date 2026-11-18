#pragma once

#include <ctime>
#include <iosfwd>
#include <string>
#include <string_view>

namespace osc
{
    class Mesh;

    struct STLMetadata final {

        explicit STLMetadata();

        explicit STLMetadata(std::string_view authoring_tool);

        std::string authoring_tool;
        std::tm creation_time;
    };

    class STL final {
    public:
        static void write(std::ostream&, const Mesh&, const STLMetadata&);
    };
}

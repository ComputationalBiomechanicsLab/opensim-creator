#pragma once

#include <ctime>
#include <iosfwd>
#include <string>
#include <string_view>

namespace osc { class Mesh; }

namespace osc
{
    struct StlMetadata final {

        explicit StlMetadata();

        explicit StlMetadata(
            std::string_view authoring_tool_
        );

        std::string authoring_tool;
        std::tm creation_time;
    };

    void write_as_stl(
        std::ostream&,
        const Mesh&,
        const StlMetadata&
    );
}

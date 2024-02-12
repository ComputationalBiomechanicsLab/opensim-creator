#pragma once

#include <ctime>
#include <iosfwd>
#include <string>
#include <string_view>

namespace osc { class Mesh; }

namespace osc
{
    struct StlMetadata final {
        explicit StlMetadata(
            std::string_view authoringTool_
        );

        std::string authoringTool;
        std::tm creationTime;
    };

    void WriteMeshAsStl(
        std::ostream&,
        Mesh const&,
        StlMetadata const&
    );
}

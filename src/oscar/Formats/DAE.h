#pragma once

#include <ctime>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>

namespace osc { struct SceneDecoration; }

namespace osc
{
    struct DAEMetadata final {
        DAEMetadata(
            std::string_view author_,
            std::string_view authoringTool_
        );

        std::string author;
        std::string authoringTool;
        std::tm creationTime;
        std::tm modificationTime;
    };

    void writeDecorationsAsDAE(
        std::ostream&,
        std::span<const SceneDecoration>,
        const DAEMetadata&
    );
}

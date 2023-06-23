#pragma once

#include <nonstd/span.hpp>

#include <ctime>
#include <iosfwd>
#include <string>

namespace osc { struct SceneDecoration; }

namespace osc
{
    struct DAEMetadata final {
        DAEMetadata();

        std::string author;
        std::string authoringTool;
        std::tm creationTime;
        std::tm modificationTime;
    };

    void WriteDecorationsAsDAE(
        std::ostream&,
        nonstd::span<SceneDecoration const>,
        DAEMetadata const& = {}
    );
}
#pragma once

#include <nonstd/span.hpp>

#include <iosfwd>
#include <string>

namespace osc { struct SceneDecoration; }

namespace osc
{
    struct DAEMetadata final {
        DAEMetadata();

        std::string author;
        std::string authoringTool;
    };

    void WriteDecorationsAsDAE(
        nonstd::span<SceneDecoration const>,
        std::ostream&,
        DAEMetadata const& = {}
    );
}
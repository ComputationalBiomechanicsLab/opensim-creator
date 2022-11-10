#pragma once

#include <nonstd/span.hpp>

#include <iosfwd>
#include <string>

namespace osc { class SceneDecoration; }

namespace osc
{
    struct DAEMetadata final {
        std::string author;
        std::string authoringTool;

        DAEMetadata();
    };

    void WriteDecorationsAsDAE(
        nonstd::span<SceneDecoration const>,
        std::ostream&,
        DAEMetadata const& = {}
    );
}
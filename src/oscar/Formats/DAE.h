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
            std::string_view authoring_tool_
        );

        std::string author;
        std::string authoring_tool;
        std::tm creation_time;
        std::tm modification_time;
    };

    void write_as_dae(
        std::ostream&,
        std::span<const SceneDecoration>,
        const DAEMetadata&
    );
}

#pragma once

#include <chrono>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>

namespace osc { struct SceneDecoration; }

namespace osc
{
    struct DAEMetadata final {

        explicit DAEMetadata();

        explicit DAEMetadata(
            std::string_view author_,
            std::string_view authoring_tool_
        );

        std::string author;
        std::string authoring_tool;
        std::chrono::zoned_seconds creation_time;
        std::chrono::zoned_seconds modification_time;
    };

    void write_as_dae(
        std::ostream&,
        std::span<const SceneDecoration>,
        const DAEMetadata& = DAEMetadata{}
    );
}

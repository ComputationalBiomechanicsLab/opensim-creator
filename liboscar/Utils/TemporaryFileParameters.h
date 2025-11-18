#pragma once

#include <string_view>

namespace osc
{
    // parameters for constructing a `TemporaryFile`
    //
    // designed for designated initializer compatibility: `TemporaryFile temporary_file({ .suffix = ".obj" });`
    struct TemporaryFileParameters final {

        // a prefix that will be prepended to the dynamic portion of the temporary file name (e.g. `${prefix}XXXXXX${suffix}`)
        std::string_view prefix = {};

        // a suffix that will be appended to the dynamic portion of the temporary file name (e.g. `${prefix}XXXXXX${suffix}`)
        std::string_view suffix = {};
    };
}

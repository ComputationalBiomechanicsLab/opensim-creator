#pragma once

#include <string>

namespace osc
{
    // Parameters for constructing a `TemporaryFile`.
    //
    // Designed for designated initializer compatibility:
    //
    //     `TemporaryFile temporary_file({ .suffix = ".obj" });`
    struct TemporaryFileParameters final {

        // A prefix that will be prepended to the dynamic portion of the temporary
        // file name (e.g. `${prefix}XXXXXX${suffix}`).
        std::string prefix{};

        // A suffix that will be appended to the dynamic portion of the temporary
        // file name (e.g. `${prefix}XXXXXX${suffix}`).
        std::string suffix{};
    };
}

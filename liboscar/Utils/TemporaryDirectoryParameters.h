#pragma once

#include <string>

namespace osc
{
    // Parameters for constructing a `TemporaryDirectory`
    //
    // Designed for designated initializer compatibility:
    //
    //     `TemporaryDirectory temporary_directory({ .suffix = "_data" });`
    struct TemporaryDirectoryParameters final {

        // A prefix that will be prepended to the dynamic portion of the temporary
        // directory name (e.g. `${prefix}XXXXXX${suffix}`).
        std::string prefix{};

        // A suffix that will be appended to the dynamic portion of the temporary
        // file name (e.g. `${prefix}XXXXXX${suffix}`).
        std::string suffix{};
    };
}

#include "virtual_filesystem.h"

#include <liboscar/utilities/exception_helpers.h>

#include <istream>
#include <iterator>
#include <string>

std::string osc::VirtualFilesystem::slurp(const ResourcePath& resource_path)
{
    auto fd = open(resource_path);
    fd.stream().exceptions(std::istream::failbit | std::istream::badbit);

    // https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring

    std::string rv;
    try {
        // reserve memory
        fd.stream().seekg(0, std::ios::end);
        if (const auto pos = fd.stream().tellg(); pos > 0) {
            rv.reserve(static_cast<size_t>(pos));
        }
        fd.stream().seekg(0, std::ios::beg);

        // then assign to it via an iterator
        rv.assign(
            (std::istreambuf_iterator<char>{fd}),
            std::istreambuf_iterator<char>{}
        );
    }
    catch (const std::exception& ex) {
        throw formatted_runtime_error("{}: error reading resource: {}", resource_path.string(), ex.what());
    }

    return rv;
}

#include "IResourceLoader.h"

#include <istream>
#include <iterator>
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>

std::string osc::IResourceLoader::slurp(ResourcePath const& rp)
{
    auto fd = open(rp);
    fd.stream().exceptions(std::istream::failbit | std::istream::badbit);

    // https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring

    std::string rv;

    try {
        // reserve memory
        fd.stream().seekg(0, std::ios::end);
        if (auto pos = fd.stream().tellg(); pos > 0) {
            rv.reserve(static_cast<size_t>(pos));
        }
        fd.stream().seekg(0, std::ios::beg);

        // then assign to it via an iterator
        rv.assign(
            (std::istreambuf_iterator<char>{fd}),
            std::istreambuf_iterator<char>{}
        );
    }
    catch (std::exception const& ex) {
        std::stringstream ss;
        ss << rp << ": error reading resource: " << ex.what();
        throw std::runtime_error{std::move(ss).str()};
    }

    return rv;
}

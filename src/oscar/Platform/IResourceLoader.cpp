#include "IResourceLoader.hpp"

#include <istream>
#include <iterator>
#include <string>

std::string osc::IResourceLoader::slurp(ResourcePath const& rp)
{
    auto fd = open(rp);

    // https://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring

    std::string rv;

    // reserve memory
    fd.stream().seekg(0, std::ios::end);
    rv.reserve(fd.stream().tellg());
    fd.stream().seekg(0, std::ios::beg);

    rv.assign(
        (std::istreambuf_iterator<char>{fd}),
         std::istreambuf_iterator<char>{}
    );

    return rv;
}

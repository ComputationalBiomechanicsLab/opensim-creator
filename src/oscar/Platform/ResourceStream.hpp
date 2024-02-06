#pragma once

#include <filesystem>
#include <iosfwd>
#include <memory>

namespace osc
{
    class ResourceStream final {
    public:
        operator std::istream& () { return *m_Handle; }
    private:
        friend class App;
        ResourceStream(std::filesystem::path const&);

        std::unique_ptr<std::istream> m_Handle;
    };
}

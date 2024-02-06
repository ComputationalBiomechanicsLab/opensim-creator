#pragma once

#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>

namespace osc
{
    class ResourceStream final {
    public:
        explicit ResourceStream(std::filesystem::path const&);

        std::string_view name() const { return m_Name; }
        std::istream& stream() const { return *m_Handle; }

        operator std::istream& () { return *m_Handle; }

    private:
        std::string m_Name;
        std::unique_ptr<std::istream> m_Handle;
    };
}

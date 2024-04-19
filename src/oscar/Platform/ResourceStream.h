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
        ResourceStream();
        explicit ResourceStream(const std::filesystem::path&);

        std::string_view name() const { return name_; }
        std::istream& stream() const { return *handle_; }

        operator std::istream& () { return *handle_; }

    private:
        std::string name_;
        std::unique_ptr<std::istream> handle_;
    };
}

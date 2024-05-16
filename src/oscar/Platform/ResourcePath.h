#pragma once

#include <concepts>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace osc
{
    class ResourcePath final {
    public:
        template<typename... Args>
        requires std::constructible_from<std::filesystem::path, Args&&...>
        ResourcePath(Args&&... args) :
            path_{std::forward<Args>(args)...}
        {}

        std::string string() const
        {
            return path_.string();
        }

        bool has_extension(std::string_view ext) const
        {
            return path_.extension() == ext;
        }

        std::string stem() const
        {
            return path_.stem().string();
        }

        friend bool operator==(const ResourcePath&, const ResourcePath&) = default;
        friend ResourcePath operator/(const ResourcePath& lhs, const ResourcePath& rhs)
        {
            return ResourcePath{lhs.path_ / rhs.path_};
        }
        friend ResourcePath operator/(const ResourcePath& lhs, std::string_view rhs)
        {
            return ResourcePath{lhs.path_ / rhs};
        }
        friend std::ostream& operator<<(std::ostream& out, const ResourcePath& resource_path)
        {
            return out << resource_path.path_;
        }
    private:
        friend struct std::hash<osc::ResourcePath>;
        std::filesystem::path path_;
    };
}

template<>
struct std::hash<osc::ResourcePath> final {
    size_t operator()(const osc::ResourcePath& resource_path) const
    {
        return std::filesystem::hash_value(resource_path.path_);
    }
};

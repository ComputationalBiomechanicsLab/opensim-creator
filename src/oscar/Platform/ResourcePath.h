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
            m_Path{std::forward<Args>(args)...}
        {}

        std::string string() const
        {
            return m_Path.string();
        }

        bool hasExtension(std::string_view ext) const
        {
            return m_Path.extension() == ext;
        }

        std::string stem() const
        {
            return m_Path.stem().string();
        }

        friend bool operator==(ResourcePath const&, ResourcePath const&) = default;
        friend ResourcePath operator/(ResourcePath const& lhs, ResourcePath const& rhs)
        {
            return ResourcePath{lhs.m_Path / rhs.m_Path};
        }
        friend ResourcePath operator/(ResourcePath const& lhs, std::string_view rhs)
        {
            return ResourcePath{lhs.m_Path / rhs};
        }
        friend std::ostream& operator<<(std::ostream& os, ResourcePath const& p)
        {
            return os << p.m_Path;
        }
    private:
        friend struct std::hash<osc::ResourcePath>;
        std::filesystem::path m_Path;
    };
}

template<>
struct std::hash<osc::ResourcePath> final {
    size_t operator()(osc::ResourcePath const& p) const
    {
        return std::filesystem::hash_value(p.m_Path);
    }
};

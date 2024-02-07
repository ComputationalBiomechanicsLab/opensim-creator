#pragma once

#include <concepts>
#include <filesystem>
#include <string>
#include <utility>

namespace osc
{
    class ResourcePath final {
    public:
        template<typename... Args>
        ResourcePath(Args&&... args)
            requires std::constructible_from<std::filesystem::path, Args&&...> :
            m_Path{std::forward<Args>(args)...}
        {}

        std::string string() const
        {
            return m_Path.string();
        }

        friend bool operator==(ResourcePath const&, ResourcePath const&) = default;
    private:
        std::filesystem::path m_Path;
    };
}

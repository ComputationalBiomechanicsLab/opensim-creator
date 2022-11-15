#pragma once

#include <memory>
#include <string_view>

namespace osc { class Shader; }

namespace osc
{
    class ShaderCache {
    public:
        ShaderCache();
        ShaderCache(ShaderCache const&) = delete;
        ShaderCache(ShaderCache&&) noexcept;
        ShaderCache& operator=(ShaderCache const&) = delete;
        ShaderCache& operator=(ShaderCache&&) noexcept;
        ~ShaderCache() noexcept;

        Shader const& get(
            std::string_view vertexShaderResource,
            std::string_view fragmentShaderResource
        );

        Shader const& get(
            std::string_view vertexShaderResource,
            std::string_view geometryShaderResource,
            std::string_view fragmentShaderResource
        );

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

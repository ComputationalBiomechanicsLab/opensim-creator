#pragma once

#include <string_view>

namespace osc { class Shader; }

namespace osc
{
    class ShaderCache {
    public:
        static Shader const& get(
            std::string_view vertexShaderResource,
            std::string_view fragmentShaderResource
        );

        static Shader const& get(
            std::string_view vertexShaderResource,
            std::string_view geometryShaderResource,
            std::string_view fragmentShaderResource
        );

        ShaderCache();
        ShaderCache(ShaderCache const&) = delete;
        ShaderCache(ShaderCache&&) noexcept;
        ShaderCache& operator=(ShaderCache const&) = delete;
        ShaderCache& operator=(ShaderCache&&) noexcept;
        ~ShaderCache() noexcept;

    private:
        class Impl;
        Impl* m_Impl;
    };
}

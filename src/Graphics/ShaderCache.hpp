#pragma once

#include <filesystem>
#include <memory>

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

        Shader const& load(
            std::filesystem::path const& vertexShader,
            std::filesystem::path const& fragmentShader
        );

        Shader const& load(
            std::filesystem::path const& vertexShader,
            std::filesystem::path const& geometryShader,
            std::filesystem::path const& fragmentShader
        );

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

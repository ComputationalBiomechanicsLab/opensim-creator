#pragma once

#include <oscar/Platform/ResourcePath.h>

#include <memory>

namespace osc { class MeshBasicMaterial; }
namespace osc { class ResourceLoader; }
namespace osc { class Shader; }

namespace osc
{
    class ShaderCache {
    public:
        explicit ShaderCache(ResourceLoader const&);
        ShaderCache(ShaderCache const&) = delete;
        ShaderCache(ShaderCache&&) noexcept;
        ShaderCache& operator=(ShaderCache const&) = delete;
        ShaderCache& operator=(ShaderCache&&) noexcept;
        ~ShaderCache() noexcept;

        Shader const& load(
            ResourcePath const& vertexShader,
            ResourcePath const& fragmentShader
        );

        Shader const& load(
            ResourcePath const& vertexShader,
            ResourcePath const& geometryShader,
            ResourcePath const& fragmentShader
        );

        MeshBasicMaterial const& basic_material() const;
        MeshBasicMaterial const& wireframe_material() const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

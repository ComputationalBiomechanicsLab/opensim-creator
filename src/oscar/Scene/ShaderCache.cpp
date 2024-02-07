#include "ShaderCache.hpp"

#include <oscar/Graphics/Shader.hpp>
#include <oscar/Platform/ResourceLoader.hpp>
#include <oscar/Utils/HashHelpers.hpp>
#include <oscar/Utils/SynchronizedValue.hpp>

#include <cstddef>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

using namespace osc;

namespace
{
    // parameters for a shader, to be used as a key into the shader cache
    struct ShaderInputs final {

        ShaderInputs(
            ResourcePath const& vertexShaderPath_,
            ResourcePath const& fragmentShaderPath_) :

            vertexShaderPath{vertexShaderPath_},
            fragmentShaderPath{fragmentShaderPath_}
        {}

        ShaderInputs(
            ResourcePath const& vertexShaderPath_,
            ResourcePath const& geometryShaderPath_,
            ResourcePath const& fragmentShaderPath_) :

            vertexShaderPath{vertexShaderPath_},
            geometryShaderPath{geometryShaderPath_},
            fragmentShaderPath{fragmentShaderPath_}
        {}

        friend bool operator==(ShaderInputs const&, ShaderInputs const&) = default;

        ResourcePath vertexShaderPath;
        ResourcePath geometryShaderPath;
        ResourcePath fragmentShaderPath;

        size_t hash = HashOf(vertexShaderPath, geometryShaderPath, fragmentShaderPath);
    };
}

template<>
struct std::hash<ShaderInputs> final {

    size_t operator()(ShaderInputs const& inputs) const
    {
        return inputs.hash;
    }
};

class osc::ShaderCache::Impl final {
public:
    explicit Impl(ResourceLoader const& resourceLoader_) :
        m_Loader{resourceLoader_}
    {}

    Shader const& load(
        ResourcePath const& vertexShader,
        ResourcePath const& fragmentShader)
    {
        ShaderInputs const key{vertexShader, fragmentShader};

        auto guard = m_Cache.lock();
        auto const it = guard->find(key);
        if (it != guard->end())
        {
            return it->second;
        }
        else
        {
            std::string const vertexShaderSrc = m_Loader.slurp(key.vertexShaderPath);
            std::string const fragmentShaderSrc = m_Loader.slurp(key.fragmentShaderPath);
            return guard->emplace_hint(it, key, Shader{vertexShaderSrc, fragmentShaderSrc})->second;
        }
    }
    Shader const& load(
        ResourcePath const& vertexShader,
        ResourcePath const& geometryShader,
        ResourcePath const& fragmentShader)
    {
        ShaderInputs const key{vertexShader, geometryShader, fragmentShader};

        auto guard = m_Cache.lock();
        auto const it = guard->find(key);
        if (it != guard->end())
        {
            return it->second;
        }
        else
        {
            std::string const vertexShaderSrc = m_Loader.slurp(key.vertexShaderPath);
            std::string const geometryShaderSrc = m_Loader.slurp(key.geometryShaderPath);
            std::string const fragmentShaderSrc = m_Loader.slurp(key.fragmentShaderPath);
            return guard->emplace_hint(it, key, Shader{vertexShaderSrc, geometryShaderSrc, fragmentShaderSrc})->second;
        }
    }
private:
    ResourceLoader m_Loader;
    SynchronizedValue<std::unordered_map<ShaderInputs, Shader>> m_Cache;
};


// public API (PIMPL)

osc::ShaderCache::ShaderCache(ResourceLoader const& resourceLoader_) :
    m_Impl{std::make_unique<Impl>(resourceLoader_)}
{
}

osc::ShaderCache::ShaderCache(ShaderCache&&) noexcept = default;
osc::ShaderCache& osc::ShaderCache::operator=(ShaderCache&&) noexcept = default;
osc::ShaderCache::~ShaderCache() noexcept = default;

Shader const& osc::ShaderCache::load(
    ResourcePath const& vertexShader,
    ResourcePath const& fragmentShader)
{
    return m_Impl->load(vertexShader, fragmentShader);
}

Shader const& osc::ShaderCache::load(
    ResourcePath const& vertexShader,
    ResourcePath const& geometryShader,
    ResourcePath const& fragmentShader)
{
    return m_Impl->load(vertexShader, geometryShader, fragmentShader);
}

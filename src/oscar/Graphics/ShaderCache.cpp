#include "ShaderCache.hpp"

#include "oscar/Graphics/Shader.hpp"
#include "oscar/Utils/FilesystemHelpers.hpp"
#include "oscar/Utils/HashHelpers.hpp"
#include "oscar/Utils/SynchronizedValue.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace
{
    // parameters for a shader, to be used as a key into the shader cache
    struct ShaderInputs final {

        ShaderInputs(
            std::filesystem::path const& vertexShaderPath_,
            std::filesystem::path const& fragmentShaderPath_) :

            vertexShaderPath{vertexShaderPath_},
            fragmentShaderPath{fragmentShaderPath_}
        {
        }

        ShaderInputs(
            std::filesystem::path vertexShaderPath_,
            std::filesystem::path geometryShaderPath_,
            std::filesystem::path fragmentShaderPath_) :

            vertexShaderPath{vertexShaderPath_},
            geometryShaderPath{geometryShaderPath_},
            fragmentShaderPath{fragmentShaderPath_}
        {
        }

        std::filesystem::path vertexShaderPath;
        std::filesystem::path geometryShaderPath;
        std::filesystem::path fragmentShaderPath;
        size_t hash = osc::HashOf(
            std::filesystem::hash_value(vertexShaderPath),
            std::filesystem::hash_value(geometryShaderPath),
            std::filesystem::hash_value(fragmentShaderPath)
        );
    };

    bool operator==(ShaderInputs const& a, ShaderInputs const& b)
    {
        return
            a.vertexShaderPath == b.vertexShaderPath &&
            a.geometryShaderPath == b.geometryShaderPath &&
            a.fragmentShaderPath == b.fragmentShaderPath;
    }
}

namespace std
{
    template<>
    struct hash<ShaderInputs> final {

        size_t operator()(ShaderInputs const& inputs) const
        {
            return inputs.hash;
        }
    };
}

class osc::ShaderCache::Impl final {
public:
    Shader const& load(
        std::filesystem::path const& vertexShader,
        std::filesystem::path const& fragmentShader)
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
            std::string const vertexShaderSrc = SlurpFileIntoString(key.vertexShaderPath);
            std::string const fragmentShaderSrc = SlurpFileIntoString(key.fragmentShaderPath);
            return guard->emplace_hint(it, key, Shader{vertexShaderSrc, fragmentShaderSrc})->second;
        }
    }
    Shader const& load(
        std::filesystem::path const& vertexShader,
        std::filesystem::path const& geometryShader,
        std::filesystem::path const& fragmentShader)
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
            std::string const vertexShaderSrc = SlurpFileIntoString(key.vertexShaderPath);
            std::string const geometryShaderSrc = SlurpFileIntoString(key.geometryShaderPath);
            std::string const fragmentShaderSrc = SlurpFileIntoString(key.fragmentShaderPath);
            return guard->emplace_hint(it, key, Shader{vertexShaderSrc, geometryShaderSrc, fragmentShaderSrc})->second;
        }
    }
private:
    SynchronizedValue<std::unordered_map<ShaderInputs, Shader>> m_Cache;
};


// public API (PIMPL)

osc::ShaderCache::ShaderCache() :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::ShaderCache::ShaderCache(ShaderCache&&) noexcept = default;
osc::ShaderCache& osc::ShaderCache::operator=(ShaderCache&&) noexcept = default;
osc::ShaderCache::~ShaderCache() noexcept = default;

osc::Shader const& osc::ShaderCache::load(
    std::filesystem::path const& vertexShader,
    std::filesystem::path const& fragmentShader)
{
    return m_Impl->load(vertexShader, fragmentShader);
}

osc::Shader const& osc::ShaderCache::load(
    std::filesystem::path const& vertexShader,
    std::filesystem::path const& geometryShader,
    std::filesystem::path const& fragmentShader)
{
    return m_Impl->load(vertexShader, geometryShader, fragmentShader);
}

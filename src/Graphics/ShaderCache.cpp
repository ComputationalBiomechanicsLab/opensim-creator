#include "ShaderCache.hpp"

#include "src/Graphics/Shader.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/FilesystemHelpers.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace
{
    struct ShaderInputs {
        std::filesystem::path vertexShaderPath;
        std::filesystem::path geometryShaderPath;
        std::filesystem::path fragmentShaderPath;
        size_t hash = osc::HashOf(std::filesystem::hash_value(vertexShaderPath), std::filesystem::hash_value(geometryShaderPath), std::filesystem::hash_value(fragmentShaderPath));

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
    class hash<ShaderInputs> final {
    public:
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
        auto lock = std::lock_guard{m_CacheMutex};

        ShaderInputs key{vertexShader, fragmentShader};
        auto [it, inserted] = m_Cache.try_emplace(key, std::unique_ptr<Shader>{});

        if (inserted)
        {
            try
            {
                std::string vertexShaderSrc = SlurpFileIntoString(key.vertexShaderPath);
                std::string fragmentShaderSrc = SlurpFileIntoString(key.fragmentShaderPath);
                it->second = std::make_unique<Shader>(vertexShaderSrc, fragmentShaderSrc);
            }
            catch (...)
            {
                m_Cache.erase(it);
                throw;
            }
        }

        return *it->second;
    }
    Shader const& load(
        std::filesystem::path const& vertexShader,
        std::filesystem::path const& geometryShader,
        std::filesystem::path const& fragmentShader)
    {
        auto lock = std::lock_guard{m_CacheMutex};

        ShaderInputs key{vertexShader, geometryShader, fragmentShader};
        auto [it, inserted] = m_Cache.try_emplace(key, std::unique_ptr<Shader>{});

        if (inserted)
        {
            try
            {
                std::string vertexShaderSrc = SlurpFileIntoString(key.vertexShaderPath);
                std::string geometryShaderSrc = SlurpFileIntoString(key.geometryShaderPath);
                std::string fragmentShaderSrc = SlurpFileIntoString(key.fragmentShaderPath);
                it->second = std::make_unique<Shader>(vertexShaderSrc, geometryShaderSrc, fragmentShaderSrc);
            }
            catch (...)
            {
                m_Cache.erase(it);
                throw;
            }
        }

        return *it->second;
    }
private:
    std::mutex m_CacheMutex;
    std::unordered_map<ShaderInputs, std::unique_ptr<Shader>> m_Cache;
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

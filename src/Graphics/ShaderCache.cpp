#include "ShaderCache.hpp"

#include "src/Graphics/Renderer.hpp"
#include "src/Platform/App.hpp"
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
    Shader const& get(std::string_view vertexShaderResource, std::string_view fragmentShaderResource)
    {
        auto lock = std::lock_guard{m_CacheMutex};

        ShaderInputs key{App::resource(vertexShaderResource), App::resource(fragmentShaderResource)};
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
    Shader const& get(std::string_view vertexShaderResource, std::string_view geometryShaderResource, std::string_view fragmentShaderResource)
    {
        auto lock = std::lock_guard{m_CacheMutex};

        ShaderInputs key{App::resource(vertexShaderResource), App::resource(geometryShaderResource), App::resource(fragmentShaderResource)};
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

// public API

osc::Shader const& osc::ShaderCache::get(std::string_view vertexShaderResource, std::string_view fragmentShaderResource)
{
    return App::shaders().m_Impl->get(std::move(vertexShaderResource), std::move(fragmentShaderResource));
}

osc::Shader const& osc::ShaderCache::get(std::string_view vertexShaderResource, std::string_view geometryShaderResource, std::string_view fragmentShaderResource)
{
    return App::shaders().m_Impl->get(std::move(vertexShaderResource), std::move(geometryShaderResource), std::move(fragmentShaderResource));
}

osc::ShaderCache::ShaderCache() : m_Impl{new Impl{}}
{
}

osc::ShaderCache::ShaderCache(ShaderCache&& tmp) noexcept : m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ShaderCache& osc::ShaderCache::operator=(ShaderCache&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ShaderCache::~ShaderCache() noexcept
{
    delete m_Impl;
}

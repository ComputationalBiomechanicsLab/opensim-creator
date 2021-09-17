#pragma once

#include "src/3D/Shader.hpp"

#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <type_traits>
#include <vector>

namespace osc {
    extern std::atomic<int> g_ShaderId;

    class ShaderCache {
        std::vector<std::unique_ptr<Shader>> m_ShaderStorage;
        std::mutex m_ShaderStorageMutex;

        template<typename TShader>
        static int getShaderId() {
            static int shaderId = ++g_ShaderId;
            return shaderId;
        }

    public:
        template<typename TShader>
        TShader& getShader() {
            static_assert(std::is_base_of_v<Shader, TShader>);

            int id = getShaderId<TShader>();

            auto guard = std::lock_guard{m_ShaderStorageMutex};
            if (id >= m_ShaderStorage.size()) {
                m_ShaderStorage.resize(id+1);
                m_ShaderStorage[id] = std::make_unique<TShader>();
            }

            Shader& s = *m_ShaderStorage[id];
            assert(typeid(s) == typeid(TShader));
            return static_cast<TShader&>(s);
        }
    };
}

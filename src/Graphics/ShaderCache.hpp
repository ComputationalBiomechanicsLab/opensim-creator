#pragma once

#include "src/Graphics/Shader.hpp"
#include "src/Utils/SynchronizedValue.hpp"

#include <atomic>
#include <cassert>
#include <memory>
#include <mutex>
#include <type_traits>
#include <vector>

namespace osc
{
    extern std::atomic<int> g_ShaderId;

    class ShaderCache {
        SynchronizedValue<std::vector<std::unique_ptr<Shader>>> m_ShaderStorage;

        template<typename TShader>
        static int getShaderId()
        {
            static int shaderId = ++g_ShaderId;
            return shaderId;
        }

    public:
        template<typename TShader>
        TShader& getShader()
        {
            static_assert(std::is_base_of_v<Shader, TShader>);

            int id = getShaderId<TShader>();

            auto guard = m_ShaderStorage.lock();
            if (id >= static_cast<int>(guard->size()))
            {
                guard->resize(id+1);
                (*guard)[id] = std::make_unique<TShader>();
            }

            Shader& s = *(*guard)[id];
            assert(typeid(s) == typeid(TShader));
            return static_cast<TShader&>(s);
        }
    };
}

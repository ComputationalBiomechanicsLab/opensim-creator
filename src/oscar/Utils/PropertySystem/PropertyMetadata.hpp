#pragma once

#include "oscar/Utils/PropertySystem/PropertyMetadata.hpp"

#include <glm/vec3.hpp>

#include <string>

namespace osc
{
    // compile-time metadata for each supported value type
    template<typename TValue>
    class PropertyMetadata;

    template<>
    class PropertyMetadata<float> final {
    public:
        static constexpr PropertyType type() { return PropertyType::Float; }
    };

    template<>
    class PropertyMetadata<glm::vec3> final {
    public:
        static constexpr PropertyType type() { return PropertyType::Vec3; }
    };

    template<>
    class PropertyMetadata<std::string> final {
    public:
        static constexpr PropertyType type() { return PropertyType::String; }
    };
}
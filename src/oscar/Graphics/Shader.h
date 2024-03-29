#pragma once

#include <oscar/Graphics/ShaderPropertyType.h>
#include <oscar/Utils/CopyOnUpdPtr.h>
#include <oscar/Utils/CStringView.h>

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string_view>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // a handle to a shader
    class Shader final {
    public:
        // throws on compile error
        Shader(
            CStringView vertexShader,
            CStringView fragmentShader
        );

        // throws on compile error
        Shader(
            CStringView vertexShader,
            CStringView geometryShader,
            CStringView fragmentShader
        );

        size_t getPropertyCount() const;
        std::optional<ptrdiff_t> findPropertyIndex(std::string_view propertyName) const;
        std::string_view getPropertyName(ptrdiff_t) const;
        ShaderPropertyType getPropertyType(ptrdiff_t) const;

        friend void swap(Shader& a, Shader& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

        friend bool operator==(Shader const&, Shader const&) = default;

    private:
        friend std::ostream& operator<<(std::ostream&, Shader const&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    std::ostream& operator<<(std::ostream&, Shader const&);
}

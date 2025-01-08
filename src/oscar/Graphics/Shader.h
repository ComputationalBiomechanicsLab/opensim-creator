#pragma once

#include <oscar/Graphics/ShaderPropertyType.h>
#include <oscar/Utils/CopyOnUpdPtr.h>

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string_view>

namespace osc
{
    // a handle to a shader
    class Shader final {
    public:
        // throws on compile error
        Shader(
            std::string_view vertex_shader_src,
            std::string_view fragment_shader_src
        );

        // throws on compile error
        Shader(
            std::string_view vertex_shader_src,
            std::string_view geometry_shader_src,
            std::string_view fragment_shader_src
        );

        size_t num_properties() const;
        std::optional<size_t> property_index(std::string_view property_name) const;
        std::string_view property_name(size_t) const;
        ShaderPropertyType property_type(size_t) const;

        friend bool operator==(const Shader&, const Shader&) = default;

    private:
        friend std::ostream& operator<<(std::ostream&, const Shader&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const Shader&);
}

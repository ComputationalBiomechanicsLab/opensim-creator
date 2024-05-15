#pragma once

#include <oscar/Graphics/ShaderPropertyType.h>
#include <oscar/Utils/CopyOnUpdPtr.h>
#include <oscar/Utils/CStringView.h>

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
            CStringView vertex_shader_src,
            CStringView fragment_shader_src
        );

        // throws on compile error
        Shader(
            CStringView vertex_shader_src,
            CStringView geometry_shader_src,
            CStringView fragment_shader_src
        );

        size_t num_properties() const;
        std::optional<ptrdiff_t> property_index(std::string_view property_name) const;
        std::string_view property_name(ptrdiff_t) const;
        ShaderPropertyType property_type(ptrdiff_t) const;

        friend bool operator==(const Shader&, const Shader&) = default;

    private:
        friend std::ostream& operator<<(std::ostream&, const Shader&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> impl_;
    };

    std::ostream& operator<<(std::ostream&, const Shader&);
}

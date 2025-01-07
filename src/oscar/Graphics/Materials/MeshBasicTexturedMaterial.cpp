#include "MeshBasicTexturedMaterial.h"

#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Utils/CStringView.h>

using namespace osc;

namespace
{
    constexpr CStringView c_vertex_shader_src = R"(
        #version 330 core

        uniform mat4 uModelMat;
        uniform mat4 uViewProjMat;

        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;

        out vec2 TexCoord;

        void main()
        {
            gl_Position = uViewProjMat * uModelMat * vec4(aPos, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    constexpr CStringView c_fragment_shader_src = R"(
        #version 330 core

        uniform sampler2D uTextureSampler;

        in vec2 TexCoord;
        out vec4 FragColor;

        void main()
        {
            FragColor = texture(uTextureSampler, TexCoord);
        }
    )";

    const StringName& texture_property_name()
    {
        static const StringName s_texture_property_name{"uTextureSampler"};
        return s_texture_property_name;
    }
}

osc::MeshBasicTexturedMaterial::MeshBasicTexturedMaterial(const Params& params) :
    Material{Shader{c_vertex_shader_src, c_fragment_shader_src}}
{
    set_texture(params.texture);
}

Texture2D osc::MeshBasicTexturedMaterial::texture() const
{
    return get<Texture2D>(texture_property_name()).value();
}

void osc::MeshBasicTexturedMaterial::set_texture(const Texture2D& texture)
{
    set(texture_property_name(), texture);
}

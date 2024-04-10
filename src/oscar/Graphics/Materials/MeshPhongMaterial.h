#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    // a material for drawing shiny meshes with specular highlights
    //
    // naming inspired by three.js's `MeshPhongMaterial`, but the implementation
    // was ported from LearnOpenGL's basic lighting tutorial
    class MeshPhongMaterial final {
    public:
        MeshPhongMaterial();

        Vec3 light_position() const { return *m_Material.get_vec3(c_LightPosPropName); }
        void set_light_position(Vec3 v) { m_Material.set_vec3(c_LightPosPropName, v); }

        Vec3 viewer_position() const { return *m_Material.get_vec3(c_ViewPosPropName); }
        void set_viewer_position(Vec3 v) { m_Material.set_vec3(c_ViewPosPropName, v); }

        Color light_color() const { return *m_Material.get_color(c_LightColorPropName); }
        void set_light_color(Color c) { m_Material.set_color(c_LightColorPropName, c); }

        Color ambient_color() const { return *m_Material.get_color(c_AmbientColorPropName); }
        void set_ambient_color(Color c) { m_Material.set_color(c_AmbientColorPropName, c); }

        Color diffuse_color() const { return *m_Material.get_color(c_DiffuseColorPropName); }
        void set_diffuse_color(Color c) { m_Material.set_color(c_DiffuseColorPropName, c); }

        Color specular_color() const { return *m_Material.get_color(c_SpecularColorPropName); }
        void set_specular_color(Color c) { m_Material.set_color(c_SpecularColorPropName, c); }

        float specular_shininess() const { return *m_Material.get_float(c_ShininessPropName); }
        void set_specular_shininess(float v) { m_Material.set_float(c_ShininessPropName, v); }

        operator const Material& () const { return m_Material; }
    private:
        static constexpr CStringView c_LightPosPropName = "uLightPos";
        static constexpr CStringView c_ViewPosPropName = "uViewPos";
        static constexpr CStringView c_LightColorPropName = "uLightColor";
        static constexpr CStringView c_AmbientColorPropName = "uAmbientColor";
        static constexpr CStringView c_DiffuseColorPropName = "uDiffuseColor";
        static constexpr CStringView c_SpecularColorPropName = "uSpecularColor";
        static constexpr CStringView c_ShininessPropName = "uShininess";
        Material m_Material;
    };
}

#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Maths/Vec3.h>

#include <string_view>

namespace osc
{
    // a material for drawing shiny meshes with specular highlights
    //
    // naming inspired by three.js's `MeshPhongMaterial`, but the implementation
    // was ported from LearnOpenGL's basic lighting tutorial
    class MeshPhongMaterial final {
    public:
        MeshPhongMaterial();

        Vec3 light_position() const { return *material_.get_vec3(c_light_pos_propname); }
        void set_light_position(Vec3 v) { material_.set_vec3(c_light_pos_propname, v); }

        Vec3 viewer_position() const { return *material_.get_vec3(c_view_pos_propname); }
        void set_viewer_position(Vec3 v) { material_.set_vec3(c_view_pos_propname, v); }

        Color light_color() const { return *material_.get_color(c_light_color_propname); }
        void set_light_color(Color c) { material_.set_color(c_light_color_propname, c); }

        Color ambient_color() const { return *material_.get_color(c_ambient_color_propname); }
        void set_ambient_color(Color c) { material_.set_color(c_ambient_color_propname, c); }

        Color diffuse_color() const { return *material_.get_color(c_diffuse_color_propname); }
        void set_diffuse_color(Color c) { material_.set_color(c_diffuse_color_propname, c); }

        Color specular_color() const { return *material_.get_color(c_specular_color_propname); }
        void set_specular_color(Color c) { material_.set_color(c_specular_color_propname, c); }

        float specular_shininess() const { return *material_.get_float(c_shininess_propname); }
        void set_specular_shininess(float v) { material_.set_float(c_shininess_propname, v); }

        operator const Material& () const { return material_; }
    private:
        static constexpr std::string_view c_light_pos_propname = "uLightPos";
        static constexpr std::string_view c_view_pos_propname = "uViewPos";
        static constexpr std::string_view c_light_color_propname = "uLightColor";
        static constexpr std::string_view c_ambient_color_propname = "uAmbientColor";
        static constexpr std::string_view c_diffuse_color_propname = "uDiffuseColor";
        static constexpr std::string_view c_specular_color_propname = "uSpecularColor";
        static constexpr std::string_view c_shininess_propname = "uShininess";
        Material material_;
    };
}

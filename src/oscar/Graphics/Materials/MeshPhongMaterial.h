#pragma once

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Maths/Vec3.h>

namespace osc
{
    struct MeshPhongMaterialParams final {
        friend bool operator==(const MeshPhongMaterialParams&, const MeshPhongMaterialParams&) = default;

        Vec3 light_position = {1.0f, 1.0f, 1.0f};
        Vec3 viewer_position = {0.0f, 0.0f, 0.0f};
        Color light_color = Color::white();
        Color ambient_color = {0.1f, 0.1f, 0.1f};
        Color diffuse_color = Color::blue();
        Color specular_color = {0.1f, 0.1f, 0.1f};
        float specular_shininess = 32.0f;
    };

    // a material for drawing shiny meshes with specular highlights
    //
    // naming inspired by three.js's `MeshPhongMaterial`, but the implementation
    // was ported from LearnOpenGL's basic lighting tutorial
    class MeshPhongMaterial final : public Material {
    public:
        using Params = MeshPhongMaterialParams;

        explicit MeshPhongMaterial(const Params& = Params{});

        Vec3 light_position() const;
        void set_light_position(const Vec3&);

        Vec3 viewer_position() const;
        void set_viewer_position(const Vec3&);

        Color light_color() const;
        void set_light_color(const Color&);

        Color ambient_color() const;
        void set_ambient_color(const Color&);

        Color diffuse_color() const;
        void set_diffuse_color(const Color&);

        Color specular_color() const;
        void set_specular_color(const Color&);

        float specular_shininess() const;
        void set_specular_shininess(float);
    };
}

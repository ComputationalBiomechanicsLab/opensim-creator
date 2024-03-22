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

        Vec3 getLightPosition() const { return *m_Material.getVec3(c_LightPosPropName); }
        void setLightPosition(Vec3 v) { m_Material.setVec3(c_LightPosPropName, v); }

        Vec3 getViewerPosition() const { return *m_Material.getVec3(c_ViewPosPropName); }
        void setViewerPosition(Vec3 v) { m_Material.setVec3(c_ViewPosPropName, v); }

        Color getLightColor() const { return *m_Material.getColor(c_LightColorPropName); }
        void setLightColor(Color c) { m_Material.setColor(c_LightColorPropName, c); }

        Color getAmbientColor() const { return *m_Material.getColor(c_AmbientColorPropName); }
        void setAmbientColor(Color c) { m_Material.setColor(c_AmbientColorPropName, c); }

        Color getDiffuseColor() const { return *m_Material.getColor(c_DiffuseColorPropName); }
        void setDiffuseColor(Color c) { m_Material.setColor(c_DiffuseColorPropName, c); }

        Color getSpecularColor() const { return *m_Material.getColor(c_SpecularColorPropName); }
        void setSpecularColor(Color c) { m_Material.setColor(c_SpecularColorPropName, c); }

        float getSpecularShininess() const { return *m_Material.getFloat(c_ShininessPropName); }
        void setSpecularShininess(float v) { m_Material.setFloat(c_ShininessPropName, v); }

        operator Material const& () const { return m_Material; }
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

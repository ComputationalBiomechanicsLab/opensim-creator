#pragma once

#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/MaterialPropertyBlock.h>
#include <oscar/Graphics/Color.h>

#include <optional>

namespace osc
{
    // a material for drawing meshes in a simple (solid-colored) way
    //
    // naming inspired by three.js's `MeshBasicMaterial`, was called
    // "SolidColorMaterial" in earlier OSCs
    class MeshBasicMaterial final {
    public:
        class PropertyBlock final {
        public:
            PropertyBlock() = default;
            explicit PropertyBlock(Color color)
            {
                property_block_.setColor(c_ColorPropName, color);
            }

            std::optional<Color> color() const { return property_block_.getColor(c_ColorPropName); }
            void set_color(Color c) { property_block_.setColor(c_ColorPropName, c); }

            operator const MaterialPropertyBlock& () const { return property_block_; }
        private:
            MaterialPropertyBlock property_block_;
        };

        MeshBasicMaterial();

        Color color() const { return *m_Material.getColor(c_ColorPropName); }
        void set_color(Color c) { m_Material.setColor(c_ColorPropName, c); }

        bool wireframe() const { return m_Material.getWireframeMode(); }
        void set_wireframe(bool v) { m_Material.setWireframeMode(v); }

        bool depth_tested() const { return m_Material.getDepthTested(); }
        void set_depth_tested(bool v) { m_Material.setDepthTested(v); }

        bool transparent() const { return m_Material.getTransparent(); }
        void set_transparent(bool v) { m_Material.setTransparent(v); }

        operator const Material& () const { return m_Material; }

    private:
        static constexpr CStringView c_ColorPropName = "uDiffuseColor";
        Material m_Material;
    };
}

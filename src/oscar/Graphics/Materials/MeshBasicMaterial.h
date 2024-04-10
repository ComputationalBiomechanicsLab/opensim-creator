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
                property_block_.set_color(c_ColorPropName, color);
            }

            std::optional<Color> color() const { return property_block_.get_color(c_ColorPropName); }
            void set_color(Color c) { property_block_.set_color(c_ColorPropName, c); }

            operator const MaterialPropertyBlock& () const { return property_block_; }
        private:
            MaterialPropertyBlock property_block_;
        };

        MeshBasicMaterial();

        Color color() const { return *m_Material.get_color(c_ColorPropName); }
        void set_color(Color c) { m_Material.set_color(c_ColorPropName, c); }

        bool wireframe() const { return m_Material.is_wireframe(); }
        void set_wireframe(bool v) { m_Material.set_wireframe(v); }

        bool depth_tested() const { return m_Material.is_depth_tested(); }
        void set_depth_tested(bool v) { m_Material.set_depth_tested(v); }

        bool transparent() const { return m_Material.is_transparent(); }
        void set_transparent(bool v) { m_Material.set_transparent(v); }

        operator const Material& () const { return m_Material; }

    private:
        static constexpr CStringView c_ColorPropName = "uDiffuseColor";
        Material m_Material;
    };
}

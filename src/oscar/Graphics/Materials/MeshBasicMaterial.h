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
                m_PropertyBlock.setColor(c_ColorPropName, color);
            }

            std::optional<Color> getColor() const { return m_PropertyBlock.getColor(c_ColorPropName); }
            void setColor(Color c) { m_PropertyBlock.setColor(c_ColorPropName, c); }

            operator MaterialPropertyBlock const& () const { return m_PropertyBlock; }
        private:
            MaterialPropertyBlock m_PropertyBlock;
        };

        MeshBasicMaterial();

        Color getColor() const { return *m_Material.getColor(c_ColorPropName); }
        void setColor(Color c) { m_Material.setColor(c_ColorPropName, c); }

        bool getWireframeMode() const { return m_Material.getWireframeMode(); }
        void setWireframeMode(bool v) { m_Material.setWireframeMode(v); }

        bool getDepthTested() const { return m_Material.getDepthTested(); }
        void setDepthTested(bool v) { m_Material.setDepthTested(v); }

        bool getTransparent() const { return m_Material.getTransparent(); }
        void setTransparent(bool v) { m_Material.setTransparent(v); }

        operator Material const& () const { return m_Material; }

    private:
        static constexpr CStringView c_ColorPropName = "uDiffuseColor";
        Material m_Material;
    };
}

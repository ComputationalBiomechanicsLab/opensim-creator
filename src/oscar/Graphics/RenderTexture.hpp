#pragma once

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Graphics/RenderTextureDescriptor.hpp>
#include <oscar/Graphics/RenderTextureFormat.hpp>
#include <oscar/Graphics/RenderTextureReadWrite.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Utils/CopyOnUpdPtr.hpp>

#include <iosfwd>
#include <memory>

namespace osc { class RenderBuffer; }

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // render texture
    //
    // a texture that can be rendered to
    class RenderTexture final {
    public:
        RenderTexture();
        explicit RenderTexture(Vec2i dimensions);
        explicit RenderTexture(RenderTextureDescriptor const&);

        Vec2i getDimensions() const;
        void setDimensions(Vec2i);

        TextureDimensionality getDimensionality() const;
        void setDimensionality(TextureDimensionality);

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        AntiAliasingLevel getAntialiasingLevel() const;
        void setAntialiasingLevel(AntiAliasingLevel);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

        RenderTextureReadWrite getReadWrite() const;
        void setReadWrite(RenderTextureReadWrite);

        void reformat(RenderTextureDescriptor const& d);

        std::shared_ptr<RenderBuffer> updColorBuffer();
        std::shared_ptr<RenderBuffer> updDepthBuffer();

        friend void swap(RenderTexture& a, RenderTexture& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

        friend bool operator==(RenderTexture const&, RenderTexture const&) = default;

        friend std::ostream& operator<<(std::ostream&, RenderTexture const&);
    private:
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    std::ostream& operator<<(std::ostream&, RenderTexture const&);
}

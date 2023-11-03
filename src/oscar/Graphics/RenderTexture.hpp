#pragma once

#include <oscar/Graphics/AntiAliasingLevel.hpp>
#include <oscar/Graphics/RenderTextureDescriptor.hpp>
#include <oscar/Graphics/RenderTextureFormat.hpp>
#include <oscar/Graphics/RenderTextureReadWrite.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Utils/CopyOnUpdPtr.hpp>

#include <iosfwd>
#include <optional>

namespace osc { class RenderBuffer; }
namespace osc { class RenderTexture; }
namespace osc { void DrawTextureAsImGuiImage(RenderTexture const&, Vec2); }

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
        RenderTexture(RenderTexture const&);
        RenderTexture(RenderTexture&&) noexcept;
        RenderTexture& operator=(RenderTexture const&);
        RenderTexture& operator=(RenderTexture&&) noexcept;
        ~RenderTexture() noexcept;

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

        friend bool operator==(RenderTexture const& lhs, RenderTexture const& rhs) noexcept
        {
            return lhs.m_Impl == rhs.m_Impl;
        }

        friend bool operator!=(RenderTexture const& lhs, RenderTexture const& rhs) noexcept
        {
            return lhs.m_Impl != rhs.m_Impl;
        }

        friend std::ostream& operator<<(std::ostream&, RenderTexture const&);
    private:
        friend void DrawTextureAsImGuiImage(RenderTexture const&, Vec2);
        void* getTextureHandleHACK() const;  // used by ImGui... for now

        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    std::ostream& operator<<(std::ostream&, RenderTexture const&);
}

#include "RendererSDFTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/ColorSpace.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/MeshGen.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/Transform.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Platform/Log.hpp"
#include "oscar/Panels/LogViewerPanel.hpp"
#include "oscar/Utils/CStringView.hpp"

#include <nonstd/span.hpp>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>  // TODO: should be moved into `Image.hpp` or similar
#include <IconsFontAwesome5.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace
{
    constexpr osc::CStringView c_TabStringID = "Experiments/SDF";

    struct CharMetadata final {
        stbtt_bakedchar storage[96];
    };

    struct FontTexture final {
        osc::Texture2D texture;
        CharMetadata metadata;
    };

    FontTexture CreateFontTexture()
    {
        std::vector<uint8_t> const ttfData = osc::App::get().slurpBinaryResource("fonts/Ruda-Bold.ttf");

        // get number of fonts in the TTF file
        int numFonts = stbtt_GetNumberOfFonts(ttfData.data());
        osc::log::info("stbtt_GetNumberOfFonts = %i", numFonts);

        // get dump info for each font in the TTF file
        for (int i = 0; i < numFonts; ++i)
        {
            int offset = stbtt_GetFontOffsetForIndex(ttfData.data(), i);
            osc::log::info("stbtt_GetFontOffsetForIndex(data, %i): %i", i, offset);

            stbtt_fontinfo info{};
            if (stbtt_InitFont(&info, ttfData.data(), i))
            {
                osc::log::info("    info.fontStart = %i", info.fontstart);
                osc::log::info("    info.numGlyphs = %i", info.numGlyphs);

                // table offsets within the TTF file
                osc::log::info("    info.loca = %i", info.loca);
                osc::log::info("    info.head = %i", info.head);
                osc::log::info("    info.glyf = %i", info.glyf);
                osc::log::info("    info.hhea = %i", info.hhea);
                osc::log::info("    info.hmtx = %i", info.hmtx);
                osc::log::info("    info.kern = %i", info.kern);
                osc::log::info("    info.gpos = %i", info.gpos);
                osc::log::info("    info.svg = %i", info.svg);

                // cmap mapping for our chosen character encoding
                osc::log::info("    info.index_map = %i", info.index_map);
                osc::log::info("    info.indexToLocFormat = %i", info.indexToLocFormat);
            }
        }

        CharMetadata glyphData;
        std::vector<uint8_t> pixels(512 * 512);

        stbtt_BakeFontBitmap(ttfData.data(), 0, 64., pixels.data(), 512, 512, 32, 96, glyphData.storage); // no guarantee this fits!
        osc::Texture2D t = osc::Texture2D
        {
            {512, 512},
            osc::TextureFormat::R8,
            pixels,
            osc::ColorSpace::sRGB
        };
        t.setFilterMode(osc::TextureFilterMode::Linear);

        return FontTexture{t, glyphData};
    }
}

class osc::RendererSDFTab::Impl final {
public:

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return c_TabStringID;
    }

    void onDraw()
    {
        printText(0.0f, 0.0f, "Hello, lack of SDF support!");
        m_LogViewer.draw();
    }

private:
    void printText(float x, float y, char const* text)
    {
        osc::Camera camera;
        camera.setCameraProjection(osc::CameraProjection::Orthographic);
        camera.setOrthographicSize(osc::App::get().dims().y);
        camera.setPosition({ 0.0f, 0.0f, 1.0f });
        camera.setNearClippingPlane(0.1f);
        camera.setFarClippingPlane(2.0f);
        camera.setBackgroundColor(Color::clear());

        m_Material.setTexture("uTexture", m_FontTexture.texture);
        m_Material.setTransparent(true);

        while (*text)
        {
            if (*text >= 32)  // max value of type is 127, which is the also conveniently the end of the table
            {
                // the Y axis is screenspace (Y goes down)
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(m_FontTexture.metadata.storage, 512,512, *text-32, &x, &y, &q, 1);  //1=opengl & d3d10+,0=d3d9

                glm::vec3 verts[] = { {q.x0, -q.y0, 0.0f}, { q.x1, -q.y0, 0.0f }, { q.x1, -q.y1, 0.0f }, { q.x0, -q.y0, 0.0f }, { q.x0, -q.y1, 0.0f }, { q.x1, -q.y1, 0.0f } };
                glm::vec2 coords[] = { { q.s0, q.t0 }, { q.s1, q.t0 }, { q.s1, q.t1 }, { q.s0, q.t0 }, { q.s0, q.t1 }, { q.s1, q.t1 } };
                uint16_t indices[] = { 0, 1, 2, 3, 4, 5 };

                osc::Mesh m;
                m.setVerts(verts);
                m.setTexCoords(coords);
                m.setIndices(indices);

                // bind texture
                osc::Graphics::DrawMesh(m, Transform{}, m_Material, camera);
            }
            ++text;
        }

        camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
        camera.renderToScreen();
    }

    UID m_TabID;

    Material m_Material
    {
        Shader
        {
            App::slurp("shaders/ExperimentSDF.vert"),
            App::slurp("shaders/ExperimentSDF.frag"),
        },
    };
    FontTexture m_FontTexture = CreateFontTexture();
    LogViewerPanel m_LogViewer{"log"};
};


// public API (PIMPL)

osc::CStringView osc::RendererSDFTab::id() noexcept
{
    return c_TabStringID;
}

osc::RendererSDFTab::RendererSDFTab(std::weak_ptr<TabHost>) :
    m_Impl{std::make_unique<Impl>()}
{
}

osc::RendererSDFTab::RendererSDFTab(RendererSDFTab&&) noexcept = default;
osc::RendererSDFTab& osc::RendererSDFTab::operator=(RendererSDFTab&&) noexcept = default;
osc::RendererSDFTab::~RendererSDFTab() noexcept = default;

osc::UID osc::RendererSDFTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::RendererSDFTab::implGetName() const
{
    return m_Impl->getName();
}

void osc::RendererSDFTab::implOnDraw()
{
    m_Impl->onDraw();
}

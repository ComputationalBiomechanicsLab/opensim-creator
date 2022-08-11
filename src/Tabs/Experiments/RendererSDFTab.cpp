#include "RendererSDFTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/MeshGen.hpp"
#include "src/Graphics/Renderer.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/Log.hpp"
#include "src/Widgets/LogViewerPanel.hpp"

#include <nonstd/span.hpp>
#include <SDL_events.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>  // TODO: should be moved into `Image.hpp` or similar
#include <IconsFontAwesome5.h>

#include <memory>
#include <string>
#include <utility>

struct CharMetadata final {
	stbtt_bakedchar storage[96];
};

struct FontTexture final {
	osc::Texture2D texture;
	CharMetadata metadata;
};

static FontTexture CreateFontTexture()
{
	std::vector<uint8_t> const ttfData = osc::App::slurpBinary("c:/windows/fonts/times.ttf");

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
	auto t = osc::Texture2D{512, 512, pixels, 1};
	t.setFilterMode(osc::TextureFilterMode::Linear);

	return FontTexture{t, glyphData};
}

class osc::RendererSDFTab::Impl final {
public:
	Impl(TabHost* parent) : m_Parent{std::move(parent)}
	{
	}

	UID getID() const
	{
		return m_ID;
	}

	CStringView getName() const
	{
		return m_Name;
	}

	TabHost* parent()
	{
		return m_Parent;
	}

	void onMount()
	{

	}

	void onUnmount()
	{

	}

	bool onEvent(SDL_Event const&)
	{
		return false;
	}

	void onTick()
	{

	}

	void onDrawMainMenu()
	{

	}

	void onDraw()
	{
		printText(0.0f, 0.0f, "Hello, lack of SDF support!");
		m_LogViewer.draw();
	}


private:
	void printText(float x, float y, char const* text)
	{
		osc::experimental::Camera camera;
		camera.setCameraProjection(osc::experimental::CameraProjection::Orthographic);
		camera.setOrthographicSize(osc::App::get().dims().y);
		camera.setPixelRect(osc::GetMainViewportWorkspaceScreenRect());
		camera.setPosition({ 0.0f, 0.0f, 1.0f });
		camera.setNearClippingPlane(0.1f);
		camera.setFarClippingPlane(2.0f);
		camera.setBackgroundColor({ 0.0f, 0.0f, 0.0f, 0.0f });

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

				osc::experimental::Mesh m;
				m.setVerts(verts);
				m.setTexCoords(coords);
				m.setIndices(indices);

				// bind texture
				osc::experimental::Graphics::DrawMesh(m, Transform{}, m_Material, camera);
			}
			++text;
		}
		camera.render();
	}

	UID m_ID;
	std::string m_Name = ICON_FA_FONT " RendererSDF";
	TabHost* m_Parent;

	// rendering stuff
	Material m_Material{ Shader{App::slurp("shaders/ExperimentSDF.vert"), App::slurp("shaders/ExperimentSDF.frag")} };
	FontTexture m_FontTexture = CreateFontTexture();

	LogViewerPanel m_LogViewer{"log"};
};


// public API

osc::RendererSDFTab::RendererSDFTab(TabHost* parent) :
	m_Impl{new Impl{std::move(parent)}}
{
}

osc::RendererSDFTab::RendererSDFTab(RendererSDFTab&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::RendererSDFTab& osc::RendererSDFTab::operator=(RendererSDFTab&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::RendererSDFTab::~RendererSDFTab() noexcept
{
	delete m_Impl;
}

osc::UID osc::RendererSDFTab::implGetID() const
{
	return m_Impl->getID();
}

osc::CStringView osc::RendererSDFTab::implGetName() const
{
	return m_Impl->getName();
}

osc::TabHost* osc::RendererSDFTab::implParent() const
{
	return m_Impl->parent();
}

void osc::RendererSDFTab::implOnMount()
{
	m_Impl->onMount();
}

void osc::RendererSDFTab::implOnUnmount()
{
	m_Impl->onUnmount();
}

bool osc::RendererSDFTab::implOnEvent(SDL_Event const& e)
{
	return m_Impl->onEvent(e);
}

void osc::RendererSDFTab::implOnTick()
{
	m_Impl->onTick();
}

void osc::RendererSDFTab::implOnDrawMainMenu()
{
	m_Impl->onDrawMainMenu();
}

void osc::RendererSDFTab::implOnDraw()
{
	m_Impl->onDraw();
}

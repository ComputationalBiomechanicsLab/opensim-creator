#include "imgui_impl_osc.hpp"

#include "osc_config.hpp"

#include "src/Graphics/Color.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Platform/App.hpp"
#include "src/Utils/Assertions.hpp"
#include "src/Utils/CStringView.hpp"
#include "src/Utils/UID.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>

#include <cstddef>
#include <cstdint>

static osc::CStringView constexpr c_VertexShader = R"(
	#version 330 core

	uniform mat4 ProjMtx;

    layout (location = 0) in vec2 Position;
    layout (location = 1) in vec2 UV;
    layout (location = 2) in vec4 Color;

    out vec2 Frag_UV;
    out vec4 Frag_Color;

    void main()
    {
        Frag_UV = UV;
        Frag_Color = Color;
        gl_Position = ProjMtx * vec4(Position.xy,0,1);
    }
)";

static osc::CStringView constexpr c_FragmentShader = R"(
	#version 330 core

    uniform sampler2D Texture;

    in vec2 Frag_UV;
    in vec4 Frag_Color;

    layout (location = 0) out vec4 Out_Color;

    void main()
    {
        Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
    }
)";

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
struct ImGui_ImplOSC_Data;
static ImGui_ImplOSC_Data* GetBackendData()
{
	if (ImGui::GetCurrentContext())
	{
		return reinterpret_cast<ImGui_ImplOSC_Data*>(ImGui::GetIO().BackendRendererUserData);
	}
	else
	{
		return nullptr;
	}
}

static osc::Texture2D CreateFontsTexture(osc::UID textureID)
{
	ImGuiIO& io = ImGui::GetIO();

	uint8_t* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(textureID.get()));

	osc::Texture2D rv
	{
		{width, height},
		osc::TextureFormat::RGBA32,
		{pixels, static_cast<size_t>(width*height*4)},
		osc::ColorSpace::sRGB,
	};
	rv.setFilterMode(osc::TextureFilterMode::Linear);
	return rv;
}

struct ImGui_ImplOSC_Data final {
	osc::UID fontTextureID;
	osc::Texture2D fontTexture = CreateFontsTexture(fontTextureID);
	osc::Material material{osc::Shader{c_VertexShader, c_FragmentShader}};
};

// basic support for multi-viewport rendering
static void RenderMultiViewportRenderWindow(ImGuiViewport* viewport, void*)
{
	if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
	{
		osc::App::upd().clearScreen(osc::Color::black());
	}
	ImGui_ImplOSC_RenderDrawData(viewport->DrawData);
}

bool ImGui_ImplOSC_Init()
{
	ImGuiIO& io = ImGui::GetIO();
	OSC_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

	// init backend data
	ImGui_ImplOSC_Data* data = new ImGui_ImplOSC_Data{};
	io.BackendRendererUserData = static_cast<void*>(data);
	io.BackendRendererName = "imgui_impl_osc";

	// tell ImGui that the OSC backend can support multiple viewports
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

	// handle multiple viewports
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		// init platform interface
		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		platformIO.Renderer_RenderWindow = RenderMultiViewportRenderWindow;
	}

	return true;
}

void ImGui_ImplOSC_Shutdown()
{
	ImGui_ImplOSC_Data* bd = GetBackendData();
	OSC_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");

	// shutdown platform interface
	ImGui::DestroyPlatformWindows();

	// destroy backend data
	ImGuiIO& io = ImGui::GetIO();
	delete bd;
	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
}

void ImGui_ImplOSC_NewFrame()
{
	OSC_ASSERT(GetBackendData() != nullptr && "Did you call ImGui_ImplOSC_Init()?");
}

static void ImGui_ImplOSC_SetupRendererState(ImDrawData* draw_data, glm::ivec2 fbDims)
{
	// alpha blending enabled
	// no face culling
	// no depth testing
	// scissor enabled
	// wireframe mode disabled
}

void ImGui_ImplOSC_RenderDrawData(ImDrawData* draw_data)
{
	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	glm::ivec2 const fbDims =
	{
		static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x),
		static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y),
	};

	if (fbDims.x <= 0 || fbDims.y <= 0)
	{
		return;
	}

	ImGui_ImplOSC_Data* bd = GetBackendData();
	OSC_ASSERT(bd != nullptr && "No renderer backend to render the draw data with, is it already shutdown?");

	

	//ImGui_ImplOpenGL3_RenderDrawData(drawData);
}
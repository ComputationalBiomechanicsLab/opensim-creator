#include "imgui_impl_oscargfx.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <glm/vec2.hpp>
#include <imgui.h>

#include <cstddef>
#include <cstdint>

namespace
{
    constexpr osc::CStringView c_VertexShader = R"(
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

    constexpr osc::CStringView c_FragmentShader = R"(
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

    osc::Texture2D CreateFontsTexture(osc::UID textureID)
    {
        ImGuiIO& io = ImGui::GetIO();

        uint8_t* pixelData = nullptr;
        glm::ivec2 dims{};
        io.Fonts->GetTexDataAsRGBA32(&pixelData, &dims.x, &dims.y);
        io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(textureID.get()));
        size_t const numBytes = static_cast<size_t>(dims.x)*static_cast<size_t>(dims.y)*static_cast<size_t>(4);

        osc::Texture2D rv
        {
            dims,
            osc::TextureFormat::RGBA32,
            osc::ColorSpace::Linear,
        };
        rv.setPixelData({pixelData, numBytes});
        rv.setFilterMode(osc::TextureFilterMode::Linear);

        return rv;
    }

    struct OscarImguiBackendData final {
        osc::UID fontTextureID;
        osc::Texture2D fontTexture = CreateFontsTexture(fontTextureID);
        osc::Material material{osc::Shader{c_VertexShader, c_FragmentShader}};
    };

    // Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
    // It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
    OscarImguiBackendData* GetBackendData()
    {
        if (ImGui::GetCurrentContext())
        {
            return reinterpret_cast<OscarImguiBackendData*>(ImGui::GetIO().BackendRendererUserData);
        }
        else
        {
            return nullptr;
        }
    }
}

bool ImGui_ImplOscarGfx_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    OSC_ASSERT(io.BackendRendererUserData == nullptr && "an oscar ImGui renderer backend is already initialized - this is a developer error (double-initialization)");

    // init backend data
    OscarImguiBackendData* data = new OscarImguiBackendData{};
    io.BackendRendererUserData = static_cast<void*>(data);
    io.BackendRendererName = "imgui_impl_osc";

    return true;
}

void ImGui_ImplOscarGfx_Shutdown()
{
    OscarImguiBackendData* bd = GetBackendData();
    OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error (double-free)");

    // shutdown platform interface
    ImGui::DestroyPlatformWindows();

    // destroy backend data
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    delete bd;
}

void ImGui_ImplOscarGfx_NewFrame()
{
    OSC_ASSERT(GetBackendData() != nullptr && "no oscar ImGui renderer backend available when creating a new frame - this is a developer error (you must call Init())");
    // TODO
}

void ImGui_ImplOscarGfx_RenderDrawData(ImDrawData* drawData)
{
    // HACK: convert all ImGui-provided colors from sRGB to linear
    //
    // this is necessary because the ImGui OpenGL backend's shaders
    // assume all color vertices and colors from textures are in
    // sRGB, but OSC can provide ImGui with linear OR sRGB textures
    // because OSC assumes the OpenGL backend is using automatic
    // color conversion support (in ImGui, it isn't)
    //
    // so what we do here is linearize all colors from ImGui and
    // always provide textures in the OSC style. The shaders in ImGui
    // then write linear color values to the screen, but because we
    // are *also* enabling GL_FRAMEBUFFER_SRGB, the OpenGL backend
    // will correctly convert those linear colors to sRGB if necessary
    // automatically
    //
    // (this shitshow is because ImGui's OpenGL backend behaves differently
    //  from OSCs - ultimately, we need an ImGui_ImplOSC backend)
    osc::ConvertDrawDataFromSRGBToLinear(*drawData);
}

#include "imgui_impl_oscargfx.hpp"

#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/CameraClearFlags.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/CullMode.hpp>
#include <oscar/Graphics/Graphics.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/SubMeshDescriptor.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/TextureFilterMode.hpp>
#include <oscar/Graphics/TextureFormat.hpp>
#include <oscar/Graphics/VertexAttribute.hpp>
#include <oscar/Graphics/VertexAttributeFormat.hpp>
#include <oscar/Graphics/VertexFormat.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Maths/Vec4.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Shims/Cpp20/bit.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/Concepts.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <oscar/Utils/StdVariantHelpers.hpp>

#include <imgui.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>

using osc::Camera;
using osc::CameraClearFlags;
using osc::ColorSpace;
using osc::CullMode;
using osc::CStringView;
using osc::Mat4;
using osc::Material;
using osc::Mesh;
using osc::MeshTopology;
using osc::MeshUpdateFlags;
using osc::Rect;
using osc::RenderTexture;
using osc::Shader;
using osc::SubMeshDescriptor;
using osc::Texture2D;
using osc::TextureFilterMode;
using osc::TextureFormat;
using osc::UID;
using osc::Vec2;
using osc::Vec2i;
using osc::Vec3;
using osc::Vec4;
using osc::VertexAttribute;
using osc::VertexAttributeFormat;

namespace
{
    constexpr CStringView c_VertexShader = R"(
        #version 330 core

        uniform mat4 uProjMat;

        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        layout (location = 3) in vec4 aColor;

        out vec2 Frag_UV;
        out vec4 Frag_Color;

        void main()
        {
            Frag_UV = aTexCoord;
            Frag_Color = aColor;
            gl_Position = uProjMat * vec4(aPos, 1.0);
        }
    )";

    constexpr CStringView c_FragmentShader = R"(
        #version 330 core

        uniform sampler2D uTexture;

        in vec2 Frag_UV;
        in vec4 Frag_Color;

        layout (location = 0) out vec4 Out_Color;

        void main()
        {
            Out_Color = Frag_Color * texture(uTexture, Frag_UV.st);
        }
    )";

    ImTextureID ToImGuiTextureID(UID id)
    {
        return osc::bit_cast<ImTextureID>(id);
    }

    UID ToUID(ImTextureID id)
    {
        return UID::FromIntUnchecked(osc::bit_cast<int64_t>(id));
    }

    Texture2D CreateFontsTexture(UID textureID)
    {
        ImGuiIO& io = ImGui::GetIO();

        uint8_t* pixelData = nullptr;
        Vec2i dims{};
        io.Fonts->GetTexDataAsRGBA32(&pixelData, &dims.x, &dims.y);
        io.Fonts->SetTexID(ToImGuiTextureID(textureID));
        size_t const numBytes = static_cast<size_t>(dims.x)*static_cast<size_t>(dims.y)*static_cast<size_t>(4);

        Texture2D rv
        {
            dims,
            TextureFormat::RGBA32,
            ColorSpace::Linear,
        };
        rv.setPixelData({pixelData, numBytes});
        rv.setFilterMode(TextureFilterMode::Linear);

        return rv;
    }

    // create a lookup table that maps sRGB color bytes to linear-space color bytes
    std::array<uint8_t, 256> CreateSRGBToLinearLUT()
    {
        std::array<uint8_t, 256> rv{};
        for (size_t i = 0; i < 256; ++i)
        {
            auto const ldrColor = static_cast<uint8_t>(i);
            float const hdrColor = osc::ToFloatingPointColorChannel(ldrColor);
            float const linearHdrColor = osc::ToLinear(hdrColor);
            uint8_t const linearLdrColor = osc::ToClamped8BitColorChannel(linearHdrColor);
            rv[i] = linearLdrColor;
        }
        return rv;
    }

    std::array<uint8_t, 256> const& GetSRGBToLinearLUT()
    {
        static std::array<uint8_t, 256> const s_LUT = CreateSRGBToLinearLUT();
        return s_LUT;
    }

    void ConvertDrawDataFromSRGBToLinear(ImDrawList& dl)
    {
        std::array<uint8_t, 256> const& lut = GetSRGBToLinearLUT();

        for (ImDrawVert& v : dl.VtxBuffer)
        {
            auto const rSRGB = static_cast<uint8_t>((v.col >> IM_COL32_R_SHIFT) & 0xFF);
            auto const gSRGB = static_cast<uint8_t>((v.col >> IM_COL32_G_SHIFT) & 0xFF);
            auto const bSRGB = static_cast<uint8_t>((v.col >> IM_COL32_B_SHIFT) & 0xFF);
            auto const aSRGB = static_cast<uint8_t>((v.col >> IM_COL32_A_SHIFT) & 0xFF);

            uint8_t const rLinear = lut[rSRGB];
            uint8_t const gLinear = lut[gSRGB];
            uint8_t const bLinear = lut[bSRGB];

            v.col =
                static_cast<ImU32>(rLinear) << IM_COL32_R_SHIFT |
                static_cast<ImU32>(gLinear) << IM_COL32_G_SHIFT |
                static_cast<ImU32>(bLinear) << IM_COL32_B_SHIFT |
                static_cast<ImU32>(aSRGB) << IM_COL32_A_SHIFT;
        }
    }

    struct OscarImguiBackendData final {

        OscarImguiBackendData()
        {
            material.setTransparent(true);
            material.setCullMode(CullMode::Off);
            material.setDepthTested(false);
            material.setWireframeMode(false);
        }

        UID fontTextureID;
        Texture2D fontTexture = CreateFontsTexture(fontTextureID);
        Material material{Shader{c_VertexShader, c_FragmentShader}};
        Camera camera;
        Mesh mesh;
        std::unordered_map<UID, std::variant<Texture2D, RenderTexture>> texturesSubmittedThisFrame = {{fontTextureID, fontTexture}};
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

    void SetupCameraViewMatrix(ImDrawData& drawData, Camera& camera)
    {
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
        float const L = drawData.DisplayPos.x;
        float const R = drawData.DisplayPos.x + drawData.DisplaySize.x;
        float const T = drawData.DisplayPos.y;
        float const B = drawData.DisplayPos.y + drawData.DisplaySize.y;

        Mat4 const projMat =
        {
            { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
            { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
            { 0.0f,         0.0f,        -1.0f,   0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
        };

        camera.setProjectionMatrixOverride(projMat);
    }

    void RenderDrawCommand(
        OscarImguiBackendData& bd,
        ImDrawData const& drawData,
        ImDrawList const&,
        Mesh& mesh,
        ImDrawCmd const& drawCommand)
    {
        OSC_ASSERT(drawCommand.UserCallback == nullptr && "user callbacks are not supported in oscar's ImGui renderer impl");

        // Will project scissor/clipping rectangles into framebuffer space
        ImVec2 clip_off = drawData.DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clip_scale = drawData.FramebufferScale; // (1,1) unless using retina display which are often (2,2)
        ImVec2 clip_min((drawCommand.ClipRect.x - clip_off.x) * clip_scale.x, (drawCommand.ClipRect.y - clip_off.y) * clip_scale.y);
        ImVec2 clip_max((drawCommand.ClipRect.z - clip_off.x) * clip_scale.x, (drawCommand.ClipRect.w - clip_off.y) * clip_scale.y);

        if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
        {
            return;
        }

        // setup clipping rectangle
        bd.camera.setClearFlags(CameraClearFlags::Nothing);
        Vec2 minflip{clip_min.x, drawData.DisplaySize.y - clip_max.y};
        Vec2 maxflip{clip_max.x, drawData.DisplaySize.y - clip_min.y};
        bd.camera.setScissorRect(Rect{minflip, maxflip});

        // setup submesh description
        SubMeshDescriptor d{drawCommand.IdxOffset, drawCommand.ElemCount, MeshTopology::Triangles};
        size_t idx = mesh.getSubMeshCount();
        mesh.pushSubMeshDescriptor(d);

        if (auto it = bd.texturesSubmittedThisFrame.find(ToUID(drawCommand.GetTexID())); it != bd.texturesSubmittedThisFrame.end())
        {
            std::visit(osc::Overload{
                [&bd](Texture2D const& t) { bd.material.setTexture("uTexture", t); },
                [&bd](RenderTexture const& t) { bd.material.setRenderTexture("uTexture", t); },
            }, it->second);
            osc::Graphics::DrawMesh(mesh, osc::Identity<Mat4>(), bd.material, bd.camera, std::nullopt, idx);
            bd.camera.renderToScreen();
        }
    }

    void RenderDrawList(
        OscarImguiBackendData& bd,
        ImDrawData const& drawData,
        ImDrawList& drawList)
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
        ConvertDrawDataFromSRGBToLinear(drawList);

        Mesh& mesh = bd.mesh;
        mesh.clear();
        mesh.setVertexBufferParams(drawList.VtxBuffer.Size, {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x2},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Color,     VertexAttributeFormat::Unorm8x4},
        });
        mesh.setVertexBufferData(std::span<ImDrawVert>{drawList.VtxBuffer.Data, static_cast<size_t>(drawList.VtxBuffer.Size)});
        mesh.setIndices({drawList.IdxBuffer.Data, static_cast<size_t>(drawList.IdxBuffer.size())}, MeshUpdateFlags::DontRecalculateBounds | MeshUpdateFlags::DontValidateIndices);

        // iterate through command buffer
        for (int i = 0; i < drawList.CmdBuffer.Size; ++i)
        {
            RenderDrawCommand(bd, drawData, drawList, mesh, drawList.CmdBuffer[i]);
        }
        mesh.clear();
    }

    template<osc::IsAnyOf<Texture2D, RenderTexture> Texture>
    ImTextureID AllocateTextureID(Texture const& texture)
    {
        OscarImguiBackendData* bd = GetBackendData();
        OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error");
        UID uid = bd->texturesSubmittedThisFrame.try_emplace(UID{}, texture).first->first;
        return ToImGuiTextureID(uid);
    }
}

bool ImGui_ImplOscarGfx_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    OSC_ASSERT(io.BackendRendererUserData == nullptr && "an oscar ImGui renderer backend is already initialized - this is a developer error (double-initialization)");

    // init backend data
    io.BackendRendererUserData = static_cast<void*>(new OscarImguiBackendData{});
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
    delete bd;  // NOLINT(cppcoreguidelines-owning-memory)
}

void ImGui_ImplOscarGfx_NewFrame()
{
    // `ImGui_ImplOpenGL3_CreateDeviceObjects` is now part of constructing `OscarImguiBackendData`

    OscarImguiBackendData* bd = GetBackendData();
    OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error");
    bd->texturesSubmittedThisFrame.clear();
    bd->texturesSubmittedThisFrame.try_emplace(bd->fontTextureID, bd->fontTexture);  // (so that all lookups can hit the same LUT)
}

void ImGui_ImplOscarGfx_RenderDrawData(ImDrawData* drawData)
{
    OscarImguiBackendData* bd = GetBackendData();
    OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error");

    SetupCameraViewMatrix(*drawData, bd->camera);
    for (int n = 0; n < drawData->CmdListsCount; ++n)
    {
        RenderDrawList(*bd, *drawData, *drawData->CmdLists[n]);
    }
}

ImTextureID ImGui_ImplOscarGfx_AllocateTextureID(Texture2D const& texture)
{
    return AllocateTextureID(texture);
}

ImTextureID ImGui_ImplOscarGfx_AllocateTextureID(osc::RenderTexture const& texture)
{
    return AllocateTextureID(texture);
}

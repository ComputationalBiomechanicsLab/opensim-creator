#include "ui_graphics_backend.h"

#include <oscar/Graphics/Camera.h>
#include <oscar/Graphics/CameraClearFlags.h>
#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/ColorSpace.h>
#include <oscar/Graphics/CullMode.h>
#include <oscar/Graphics/Graphics.h>
#include <oscar/Graphics/Material.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Shader.h>
#include <oscar/Graphics/SubMeshDescriptor.h>
#include <oscar/Graphics/Texture2D.h>
#include <oscar/Graphics/TextureFilterMode.h>
#include <oscar/Graphics/TextureFormat.h>
#include <oscar/Graphics/Unorm8.h>
#include <oscar/Graphics/VertexAttribute.h>
#include <oscar/Graphics/VertexAttributeFormat.h>
#include <oscar/Graphics/VertexFormat.h>
#include <oscar/Maths/MatFunctions.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/Rect.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Maths/Vec4.h>
#include <oscar/Platform/App.h>
#include <oscar/Shims/Cpp20/bit.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/Concepts.h>
#include <oscar/Utils/CStringView.h>
#include <oscar/Utils/StdVariantHelpers.h>
#include <oscar/Utils/UID.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <unordered_map>
#include <variant>

namespace graphics = osc::graphics;
namespace cpp20 = osc::cpp20;
using namespace osc;

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
        return cpp20::bit_cast<ImTextureID>(id);
    }

    UID ToUID(ImTextureID id)
    {
        return UID::FromIntUnchecked(cpp20::bit_cast<UID::element_type>(id));
    }

    Texture2D CreateFontsTexture(UID textureID)
    {
        ImGuiIO& io = ui::GetIO();

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
            auto const ldrColor = Unorm8{static_cast<uint8_t>(i)};
            float const hdrColor = ldrColor.normalized_value();
            float const linearHdrColor = toLinear(hdrColor);
            rv[i] = Unorm8{linearHdrColor}.raw_value();
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
            return std::launder(reinterpret_cast<OscarImguiBackendData*>(ui::GetIO().BackendRendererUserData));
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
        Vec2 clip_off = drawData.DisplayPos;         // (0,0) unless using multi-viewports
        Vec2 clip_scale = drawData.FramebufferScale; // (1,1) unless using retina display which are often (2,2)
        Vec2 clip_min((drawCommand.ClipRect.x - clip_off.x) * clip_scale.x, (drawCommand.ClipRect.y - clip_off.y) * clip_scale.y);
        Vec2 clip_max((drawCommand.ClipRect.z - clip_off.x) * clip_scale.x, (drawCommand.ClipRect.w - clip_off.y) * clip_scale.y);

        if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
        {
            return;
        }

        // setup clipping rectangle
        bd.camera.setClearFlags(CameraClearFlags::Nothing);
        Vec2 minflip{clip_min.x, (drawData.FramebufferScale.y * drawData.DisplaySize.y) - clip_max.y};
        Vec2 maxflip{clip_max.x, (drawData.FramebufferScale.y * drawData.DisplaySize.y) - clip_min.y};
        bd.camera.setScissorRect(Rect{minflip, maxflip});

        // setup submesh description
        SubMeshDescriptor d{drawCommand.IdxOffset, drawCommand.ElemCount, MeshTopology::Triangles};
        size_t idx = mesh.getSubMeshCount();
        mesh.pushSubMeshDescriptor(d);

        if (auto const* texture = try_find(bd.texturesSubmittedThisFrame, ToUID(drawCommand.GetTexID())))
        {
            std::visit(Overload{
                [&bd](Texture2D const& t) { bd.material.setTexture("uTexture", t); },
                [&bd](RenderTexture const& t) { bd.material.setRenderTexture("uTexture", t); },
            }, *texture);
            graphics::drawMesh(mesh, identity<Mat4>(), bd.material, bd.camera, std::nullopt, idx);
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

    template<IsAnyOf<Texture2D, RenderTexture> Texture>
    ImTextureID AllocateTextureID(Texture const& texture)
    {
        OscarImguiBackendData* bd = GetBackendData();
        OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error");
        UID uid = bd->texturesSubmittedThisFrame.try_emplace(UID{}, texture).first->first;
        return ToImGuiTextureID(uid);
    }
}

bool osc::ui::graphics_backend::Init()
{
    ImGuiIO& io = ui::GetIO();
    OSC_ASSERT(io.BackendRendererUserData == nullptr && "an oscar ImGui renderer backend is already initialized - this is a developer error (double-initialization)");

    // init backend data
    io.BackendRendererUserData = static_cast<void*>(new OscarImguiBackendData{});
    io.BackendRendererName = "imgui_impl_osc";

    return true;
}

void osc::ui::graphics_backend::Shutdown()
{
    OscarImguiBackendData* bd = GetBackendData();
    OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error (double-free)");

    // shutdown platform interface
    ImGui::DestroyPlatformWindows();

    // destroy backend data
    ImGuiIO& io = ui::GetIO();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    delete bd;  // NOLINT(cppcoreguidelines-owning-memory)
}

void osc::ui::graphics_backend::NewFrame()
{
    // `ImGui_ImplOpenGL3_CreateDeviceObjects` is now part of constructing `OscarImguiBackendData`

    OscarImguiBackendData* bd = GetBackendData();
    OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error");
    bd->texturesSubmittedThisFrame.clear();
    bd->texturesSubmittedThisFrame.try_emplace(bd->fontTextureID, bd->fontTexture);  // (so that all lookups can hit the same LUT)
}

void osc::ui::graphics_backend::RenderDrawData(ImDrawData* drawData)
{
    OscarImguiBackendData* bd = GetBackendData();
    OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error");

    SetupCameraViewMatrix(*drawData, bd->camera);
    for (int n = 0; n < drawData->CmdListsCount; ++n)
    {
        RenderDrawList(*bd, *drawData, *drawData->CmdLists[n]);
    }
}

ImTextureID osc::ui::graphics_backend::AllocateTextureID(Texture2D const& texture)
{
    return ::AllocateTextureID(texture);
}

ImTextureID osc::ui::graphics_backend::AllocateTextureID(RenderTexture const& texture)
{
    return ::AllocateTextureID(texture);
}

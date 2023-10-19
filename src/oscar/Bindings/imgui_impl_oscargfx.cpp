#include "imgui_impl_oscargfx.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/Camera.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/ColorSpace.hpp>
#include <oscar/Graphics/CullMode.hpp>
#include <oscar/Graphics/Material.hpp>
#include <oscar/Graphics/Shader.hpp>
#include <oscar/Graphics/Texture2D.hpp>
#include <oscar/Graphics/TextureFilterMode.hpp>
#include <oscar/Graphics/TextureFormat.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>

#include <cstddef>
#include <cstdint>

namespace
{
    constexpr osc::CStringView c_VertexShader = R"(
        #version 330 core

        uniform mat4 uProjMat;

        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        layout (location = 2) in vec4 aColor;

        out vec2 Frag_UV;
        out vec4 Frag_Color;

        void main()
        {
            Frag_UV = aTexCoord;
            Frag_Color = aColor;
            gl_Position = uProjMat * vec4(aPos, 1.0);
        }
    )";

    constexpr osc::CStringView c_FragmentShader = R"(
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

        OscarImguiBackendData()
        {
            material.setTransparent(true);
            material.setCullMode(osc::CullMode::Off);
            material.setDepthTested(false);
            material.setWireframeMode(false);
        }

        osc::UID fontTextureID;
        osc::Texture2D fontTexture = CreateFontsTexture(fontTextureID);
        osc::Material material{osc::Shader{c_VertexShader, c_FragmentShader}};
        osc::Camera camera;
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

    void SetupCameraViewMatrix(ImDrawData& drawData, osc::Camera& camera)
    {
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
        float const L = drawData.DisplayPos.x;
        float const R = drawData.DisplayPos.x + drawData.DisplaySize.x;
        float const T = drawData.DisplayPos.y;
        float const B = drawData.DisplayPos.y + drawData.DisplaySize.y;

        glm::mat4 const projMat =
        {
            { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
            { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
            { 0.0f,         0.0f,        -1.0f,   0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
        };

        camera.setProjectionMatrixOverride(projMat);
    }

    void RenderDrawCommand(
        OscarImguiBackendData&,
        ImDrawData const& drawData,
        ImDrawList const&,
        ImDrawCmd const& drawCommand)
    {
        OSC_ASSERT(drawCommand.UserCallback == nullptr && "user callbacks are not supported in oscar's ImGui renderer impl");

        // imgui_impl_opengl3.cpp L530-605

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
        // update camera with the clipping rectangle
        // flush the draw command
        // set texture ID  --> drawCommand.GetTexID();
    }

    void RenderDrawList(
        OscarImguiBackendData& bd,
        ImDrawData const& drawData,
        ImDrawList const& drawList)
    {
        // upload verticies and indices
        // GL_CALL(glBufferData(GL_ARRAY_BUFFER, vtx_buffer_size, (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW));
        // GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx_buffer_size, (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW));

        // iterate through command buffer
        for (int i = 0; i < drawList.CmdBuffer.Size; ++i)
        {
            RenderDrawCommand(bd, drawData, drawList, drawList.CmdBuffer[i]);
        }
    }
}

bool ImGui_ImplOscarGfx_Init()
{
    ImGuiIO& io = ImGui::GetIO();
    OSC_ASSERT(io.BackendRendererUserData == nullptr && "an oscar ImGui renderer backend is already initialized - this is a developer error (double-initialization)");

    // init backend data
    OscarImguiBackendData* bd = new OscarImguiBackendData{};
    io.BackendRendererUserData = static_cast<void*>(bd);
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
    // `ImGui_ImplOpenGL3_CreateDeviceObjects` is now part of constructing `OscarImguiBackendData`
}

void ImGui_ImplOscarGfx_RenderDrawData(ImDrawData* drawData)
{
    OscarImguiBackendData* bd = GetBackendData();
    OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error");

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
    osc::ConvertDrawDataFromSRGBToLinear(*drawData);  // TODO: do this as part of encoding into `osc::Mesh`

    SetupCameraViewMatrix(*drawData, bd->camera);
    for (int n = 0; n < drawData->CmdListsCount; ++n)
    {
        RenderDrawList(*bd, *drawData, *drawData->CmdLists[n]);
    }
}

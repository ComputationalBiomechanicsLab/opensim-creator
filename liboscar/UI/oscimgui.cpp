#include "oscimgui.h"

#include <liboscar/Graphics/Camera.h>
#include <liboscar/Graphics/CameraClearFlags.h>
#include <liboscar/Graphics/Color.h>
#include <liboscar/Graphics/ColorSpace.h>
#include <liboscar/Graphics/CullMode.h>
#include <liboscar/Graphics/Graphics.h>
#include <liboscar/Graphics/Material.h>
#include <liboscar/Graphics/Mesh.h>
#include <liboscar/Graphics/RenderTexture.h>
#include <liboscar/Graphics/Shader.h>
#include <liboscar/Graphics/SubMeshDescriptor.h>
#include <liboscar/Graphics/Texture2D.h>
#include <liboscar/Graphics/TextureFilterMode.h>
#include <liboscar/Graphics/TextureFormat.h>
#include <liboscar/Graphics/Unorm8.h>
#include <liboscar/Graphics/VertexAttribute.h>
#include <liboscar/Graphics/VertexAttributeFormat.h>
#include <liboscar/Graphics/VertexFormat.h>
#include <liboscar/Maths/Angle.h>
#include <liboscar/Maths/ClosedInterval.h>
#include <liboscar/Maths/CollisionTests.h>
#include <liboscar/Maths/CommonFunctions.h>
#include <liboscar/Maths/EulerAngles.h>
#include <liboscar/Maths/GeometricFunctions.h>
#include <liboscar/Maths/Mat4.h>
#include <liboscar/Maths/MatFunctions.h>
#include <liboscar/Maths/MathHelpers.h>
#include <liboscar/Maths/PolarPerspectiveCamera.h>
#include <liboscar/Maths/Quat.h>
#include <liboscar/Maths/Rect.h>
#include <liboscar/Maths/RectFunctions.h>
#include <liboscar/Maths/Transform.h>
#include <liboscar/Maths/Vec2.h>
#include <liboscar/Maths/Vec3.h>
#include <liboscar/Maths/Vec4.h>
#include <liboscar/Maths/VecFunctions.h>
#include <liboscar/Platform/App.h>
#include <liboscar/Platform/AppSettings.h>
#include <liboscar/Platform/Cursor.h>
#include <liboscar/Platform/CursorShape.h>
#include <liboscar/Platform/Events.h>
#include <liboscar/Platform/os.h>
#include <liboscar/Platform/PhysicalKeyModifier.h>
#include <liboscar/Platform/ResourceLoader.h>
#include <liboscar/Platform/ResourcePath.h>
#include <liboscar/Platform/WindowID.h>
#include <liboscar/Shims/Cpp23/ranges.h>
#include <liboscar/UI/Detail/ImGuizmo.h>
#include <liboscar/Utils/Algorithms.h>
#include <liboscar/Utils/Assertions.h>
#include <liboscar/Utils/Conversion.h>
#include <liboscar/Utils/CStringView.h>
#include <liboscar/Utils/EnumHelpers.h>
#include <liboscar/Utils/Flags.h>
#include <liboscar/Utils/Perf.h>
#include <liboscar/Utils/ScopeExit.h>
#include <liboscar/Utils/StdVariantHelpers.h>
#include <liboscar/Utils/UID.h>

#include <ankerl/unordered_dense.h>
#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const osc::Vec4& v) { x = v.x; y = v.y; z = v.z; w = v.w; }  \
        operator osc::Vec4() const { return osc::Vec4(x, y, z, w); }        \
        ImVec4(const osc::Color& v) { x = v.r; y = v.g; z = v.b; w = v.a; } \
        operator osc::Color() const { return osc::Color{x, y, z, w};        }

#define IM_VEC2_CLASS_EXTRA                                                 \
         ImVec2(const osc::Vec2& f) { x = f.x; y = f.y; }                   \
         operator osc::Vec2() const { return osc::Vec2(x,y); }
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <implot.h>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "liboscar/Platform/Log.h"
#include "liboscar/Utils/StringHelpers.h"

namespace plot = osc::ui::plot;
namespace rgs = std::ranges;
using namespace osc::literals;
using namespace osc;

template<>
struct osc::Converter<ImGuiMouseCursor, CursorShape> final {
    CursorShape operator()(ImGuiMouseCursor cursor) const
    {
        static_assert(ImGuiMouseCursor_COUNT == 11);

        switch (cursor) {
        case ImGuiMouseCursor_None:       return CursorShape::Hidden;
        case ImGuiMouseCursor_Arrow:      return CursorShape::Arrow;
        case ImGuiMouseCursor_TextInput:  return CursorShape::IBeam;
        case ImGuiMouseCursor_ResizeAll:  return CursorShape::ResizeAll;
        case ImGuiMouseCursor_ResizeNS:   return CursorShape::ResizeVertical;
        case ImGuiMouseCursor_ResizeEW:   return CursorShape::ResizeHorizontal;
        case ImGuiMouseCursor_ResizeNESW: return CursorShape::ResizeDiagonalNESW;
        case ImGuiMouseCursor_ResizeNWSE: return CursorShape::ResizeDiagonalNWSE;
        case ImGuiMouseCursor_Hand:       return CursorShape::PointingHand;
        case ImGuiMouseCursor_Wait:       return CursorShape::Wait;
        case ImGuiMouseCursor_Progress:   return CursorShape::Progress;
        case ImGuiMouseCursor_NotAllowed: return CursorShape::Forbidden;
        default:                          return CursorShape::Arrow;
        }
    }
};

template<>
struct osc::Converter<Key, ImGuiKey> final {
    ImGuiKey operator()(Key key) const
    {
        static_assert(num_options<Key>() == 120);

        switch (key) {
        case Key::Tab: return ImGuiKey_Tab;
        case Key::LeftArrow: return ImGuiKey_LeftArrow;
        case Key::RightArrow: return ImGuiKey_RightArrow;
        case Key::UpArrow: return ImGuiKey_UpArrow;
        case Key::DownArrow: return ImGuiKey_DownArrow;
        case Key::PageUp: return ImGuiKey_PageUp;
        case Key::PageDown: return ImGuiKey_PageDown;
        case Key::Home: return ImGuiKey_Home;
        case Key::End: return ImGuiKey_End;
        case Key::Insert: return ImGuiKey_Insert;
        case Key::Delete: return ImGuiKey_Delete;
        case Key::Backspace: return ImGuiKey_Backspace;
        case Key::Space: return ImGuiKey_Space;
        case Key::Return: return ImGuiKey_Enter;
        case Key::Escape: return ImGuiKey_Escape;
        case Key::Apostrophe: return ImGuiKey_Apostrophe;
        case Key::Comma: return ImGuiKey_Comma;
        case Key::Minus: return ImGuiKey_Minus;
        case Key::Period: return ImGuiKey_Period;
        case Key::Slash: return ImGuiKey_Slash;
        case Key::Semicolon: return ImGuiKey_Semicolon;
        case Key::Equals: return ImGuiKey_Equal;
        case Key::LeftBracket: return ImGuiKey_LeftBracket;
        case Key::Backslash: return ImGuiKey_Backslash;
        case Key::RightBracket: return ImGuiKey_RightBracket;
        case Key::Grave: return ImGuiKey_GraveAccent;
        case Key::CapsLock: return ImGuiKey_CapsLock;
        case Key::ScrollLock: return ImGuiKey_ScrollLock;
        case Key::NumLockClear: return ImGuiKey_NumLock;
        case Key::PrintScreen: return ImGuiKey_PrintScreen;
        case Key::Pause: return ImGuiKey_Pause;
        case Key::Keypad0: return ImGuiKey_Keypad0;
        case Key::Keypad1: return ImGuiKey_Keypad1;
        case Key::Keypad2: return ImGuiKey_Keypad2;
        case Key::Keypad3: return ImGuiKey_Keypad3;
        case Key::Keypad4: return ImGuiKey_Keypad4;
        case Key::Keypad5: return ImGuiKey_Keypad5;
        case Key::Keypad6: return ImGuiKey_Keypad6;
        case Key::Keypad7: return ImGuiKey_Keypad7;
        case Key::Keypad8: return ImGuiKey_Keypad8;
        case Key::Keypad9: return ImGuiKey_Keypad9;
        case Key::KeypadPeriod: return ImGuiKey_KeypadDecimal;
        case Key::KeypadDivide: return ImGuiKey_KeypadDivide;
        case Key::KeypadMultiply: return ImGuiKey_KeypadMultiply;
        case Key::KeypadMinus: return ImGuiKey_KeypadSubtract;
        case Key::KeypadPlus: return ImGuiKey_KeypadAdd;
        case Key::KeypadEnter: return ImGuiKey_KeypadEnter;
        case Key::KeypadEquals: return ImGuiKey_KeypadEqual;
        case Key::LeftCtrl: return ImGuiKey_LeftCtrl;
        case Key::LeftShift: return ImGuiKey_LeftShift;
        case Key::LeftAlt: return ImGuiKey_LeftAlt;
        case Key::LeftGui: return ImGuiKey_LeftSuper;
        case Key::RightCtrl: return ImGuiKey_RightCtrl;
        case Key::RightShift: return ImGuiKey_RightShift;
        case Key::RightAlt: return ImGuiKey_RightAlt;
        case Key::RightGui: return ImGuiKey_RightSuper;
        case Key::Application: return ImGuiKey_Menu;
        case Key::_0: return ImGuiKey_0;
        case Key::_1: return ImGuiKey_1;
        case Key::_2: return ImGuiKey_2;
        case Key::_3: return ImGuiKey_3;
        case Key::_4: return ImGuiKey_4;
        case Key::_5: return ImGuiKey_5;
        case Key::_6: return ImGuiKey_6;
        case Key::_7: return ImGuiKey_7;
        case Key::_8: return ImGuiKey_8;
        case Key::_9: return ImGuiKey_9;
        case Key::A: return ImGuiKey_A;
        case Key::B: return ImGuiKey_B;
        case Key::C: return ImGuiKey_C;
        case Key::D: return ImGuiKey_D;
        case Key::E: return ImGuiKey_E;
        case Key::F: return ImGuiKey_F;
        case Key::G: return ImGuiKey_G;
        case Key::H: return ImGuiKey_H;
        case Key::I: return ImGuiKey_I;
        case Key::J: return ImGuiKey_J;
        case Key::K: return ImGuiKey_K;
        case Key::L: return ImGuiKey_L;
        case Key::M: return ImGuiKey_M;
        case Key::N: return ImGuiKey_N;
        case Key::O: return ImGuiKey_O;
        case Key::P: return ImGuiKey_P;
        case Key::Q: return ImGuiKey_Q;
        case Key::R: return ImGuiKey_R;
        case Key::S: return ImGuiKey_S;
        case Key::T: return ImGuiKey_T;
        case Key::U: return ImGuiKey_U;
        case Key::V: return ImGuiKey_V;
        case Key::W: return ImGuiKey_W;
        case Key::X: return ImGuiKey_X;
        case Key::Y: return ImGuiKey_Y;
        case Key::Z: return ImGuiKey_Z;
        case Key::F1: return ImGuiKey_F1;
        case Key::F2: return ImGuiKey_F2;
        case Key::F3: return ImGuiKey_F3;
        case Key::F4: return ImGuiKey_F4;
        case Key::F5: return ImGuiKey_F5;
        case Key::F6: return ImGuiKey_F6;
        case Key::F7: return ImGuiKey_F7;
        case Key::F8: return ImGuiKey_F8;
        case Key::F9: return ImGuiKey_F9;
        case Key::F10: return ImGuiKey_F10;
        case Key::F11: return ImGuiKey_F11;
        case Key::F12: return ImGuiKey_F12;
        case Key::F13: return ImGuiKey_F13;
        case Key::F14: return ImGuiKey_F14;
        case Key::F15: return ImGuiKey_F15;
        case Key::F16: return ImGuiKey_F16;
        case Key::F17: return ImGuiKey_F17;
        case Key::F18: return ImGuiKey_F18;
        case Key::F19: return ImGuiKey_F19;
        case Key::F20: return ImGuiKey_F20;
        case Key::F21: return ImGuiKey_F21;
        case Key::F22: return ImGuiKey_F22;
        case Key::F23: return ImGuiKey_F23;
        case Key::F24: return ImGuiKey_F24;
        case Key::AppBack: return ImGuiKey_AppBack;
        case Key::AppForward: return ImGuiKey_AppForward;
        default: return ImGuiKey_None;
        }
    }
};

static_assert(osc::ui::gizmo_annotation_offset() == ImGuizmo::AnnotationOffset());

namespace
{
    constexpr float c_default_base_font_pixel_size = 15.0f;

    constexpr std::string_view c_ui_vertex_shader_src = R"(
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

    constexpr std::string_view c_ui_fragment_shader_src = R"(
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

    ImTextureID to_imgui_texture_id(UID id)
    {
        static_assert(sizeof(decltype(id.get())) <= sizeof(ImTextureID));
        return static_cast<ImTextureID>(id.get());
    }

    UID to_uid(ImTextureID id)
    {
        return UID::from_int_unchecked(static_cast<UID::element_type>(id));
    }

    Texture2D create_font_texture(UID texture_id)
    {
        ImGuiIO& io = ImGui::GetIO();

        uint8_t* pixel_data = nullptr;
        Vec2i dims;
        io.Fonts->GetTexDataAsRGBA32(&pixel_data, &dims.x, &dims.y);
        io.Fonts->SetTexID(to_imgui_texture_id(texture_id));
        const size_t num_bytes = static_cast<size_t>(dims.x)*static_cast<size_t>(dims.y)*4uz;

        Texture2D rv{
            dims,
            TextureFormat::RGBA32,
            ColorSpace::Linear,
        };
        rv.set_pixel_data({pixel_data, num_bytes});
        rv.set_filter_mode(TextureFilterMode::Linear);
        io.Fonts->ClearTexData();  // it's not needed by ImGui: it'll use the GPU texture

        return rv;
    }

    // Returns a lookup table that maps sRGB color bytes to linear-space color bytes
    std::array<uint8_t, 256> create_srgb_to_linear_lut()
    {
        std::array<uint8_t, 256> rv{};
        for (size_t i = 0; i < 256; ++i) {
            const auto ldr_color = Unorm8{static_cast<uint8_t>(i)};
            const float hdr_color = ldr_color.normalized_value();
            const float linear_hdr_color = to_linear_colorspace(hdr_color);
            rv[i] = Unorm8{linear_hdr_color}.raw_value();
        }
        return rv;
    }

    const std::array<uint8_t, 256>& get_srgc_to_linear_lut_singleton()
    {
        static const std::array<uint8_t, 256> s_srgb_to_linear_lut = create_srgb_to_linear_lut();
        return s_srgb_to_linear_lut;
    }

    void convert_draw_data_from_srgb_to_linear(ImDrawList& draw_list)
    {
        const std::array<uint8_t, 256>& lut = get_srgc_to_linear_lut_singleton();

        for (ImDrawVert& v : draw_list.VtxBuffer) {
            const auto r_srgb = static_cast<uint8_t>((v.col >> IM_COL32_R_SHIFT) & 0xFF);
            const auto g_srgb = static_cast<uint8_t>((v.col >> IM_COL32_G_SHIFT) & 0xFF);
            const auto b_srgb = static_cast<uint8_t>((v.col >> IM_COL32_B_SHIFT) & 0xFF);
            const auto alpha = static_cast<uint8_t>((v.col >> IM_COL32_A_SHIFT) & 0xFF);

            const uint8_t r_linear = lut[r_srgb];
            const uint8_t g_linear = lut[g_srgb];
            const uint8_t b_linear = lut[b_srgb];

            v.col =
                static_cast<ImU32>(r_linear) << IM_COL32_R_SHIFT |
                static_cast<ImU32>(g_linear) << IM_COL32_G_SHIFT |
                static_cast<ImU32>(b_linear) << IM_COL32_B_SHIFT |
                static_cast<ImU32>(alpha) << IM_COL32_A_SHIFT;
        }
    }

    struct UiGraphicsContextData final {

        UiGraphicsContextData()
        {
            ui_material.set_transparent(true);
            ui_material.set_cull_mode(CullMode::Off);
            ui_material.set_depth_tested(false);
            ui_material.set_wireframe(false);
        }

        UID font_texture_id;
        std::optional<Texture2D> font_texture;
        Material ui_material{Shader{c_ui_vertex_shader_src, c_ui_fragment_shader_src}};
        Camera camera;
        Mesh mesh;
        ankerl::unordered_dense::map<UID, std::variant<Texture2D, RenderTexture>> textures_allocated_this_frame;
    };

    // Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
    UiGraphicsContextData* get_graphics_backend_data()
    {
        if (ImGui::GetCurrentContext()) {
            return static_cast<UiGraphicsContextData*>(ImGui::GetIO().BackendRendererUserData);
        }
        else {
            return nullptr;
        }
    }

    void setup_camera_view_matrix(const ImDrawData& draw_data, Camera& camera)
    {
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
        const float L = draw_data.DisplayPos.x;
        const float R = draw_data.DisplayPos.x + draw_data.DisplaySize.x;
        const float T = draw_data.DisplayPos.y;
        const float B = draw_data.DisplayPos.y + draw_data.DisplaySize.y;

        const Mat4 projection_matrix = {
            { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
            { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
            { 0.0f,         0.0f,        -1.0f,   0.0f },
            { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
        };

        camera.set_projection_matrix_override(projection_matrix);
    }

    void render_draw_command(
        UiGraphicsContextData& bd,
        const ImDrawData& draw_data,
        const ImDrawList&,
        Mesh& mesh,
        const ImDrawCmd& draw_command,
        RenderTexture* maybe_target)
    {
        OSC_ASSERT(draw_command.UserCallback == nullptr && "user callbacks are not supported in oscar's ImGui renderer impl");

        // Project scissor/clipping rectangles from ui space, in device-independent
        // pixels, into screenspace, also in device-independent pixels.
        const Vec2 clip_off = draw_data.DisplayPos;         // (0,0) unless using multi-viewports
        const Vec2 clip_min(draw_command.ClipRect.x - clip_off.x, draw_command.ClipRect.y - clip_off.y);
        const Vec2 clip_max(draw_command.ClipRect.z - clip_off.x, draw_command.ClipRect.w - clip_off.y);

        if (clip_max.x <= clip_min.x or clip_max.y <= clip_min.y) {
            return;
        }
        const Vec2 minflip{clip_min.x, (draw_data.DisplaySize.y) - clip_max.y};
        const Vec2 maxflip{clip_max.x, (draw_data.DisplaySize.y) - clip_min.y};

        // setup clipping rectangle
        bd.camera.set_clear_flags(CameraClearFlag::None);
        bd.camera.set_scissor_rect(Rect{minflip, maxflip});

        // setup sub-mesh description
        const size_t sub_mesh_index = mesh.num_submesh_descriptors();
        mesh.push_submesh_descriptor(SubMeshDescriptor{
            draw_command.IdxOffset,
            draw_command.ElemCount,
            MeshTopology::Triangles,
            draw_command.VtxOffset
        });

        // setup texture binding (it's almost always the font texture)
        if (const auto* texture = lookup_or_nullptr(bd.textures_allocated_this_frame, to_uid(draw_command.GetTexID()))) {
            std::visit(Overload{
                [&bd](const auto& texture) { bd.ui_material.set("uTexture", texture); },
            }, *texture);
        }
        else if (bd.font_texture) {
            // this is a sane fallback for custom drawlists, which might not have set
            // a texture ID (imgui always sets it).
            bd.ui_material.set("uTexture", *bd.font_texture);
        }

        // draw
        graphics::draw(mesh, identity<Mat4>(), bd.ui_material, bd.camera, std::nullopt, sub_mesh_index);

        // flush draw queue to output
        if (maybe_target) {
            bd.camera.render_to(*maybe_target);
        }
        else {
            bd.camera.render_to_main_window();
        }
    }

    void render_drawlist(
        UiGraphicsContextData& bd,
        const ImDrawData& draw_data,
        ImDrawList& draw_list,
        RenderTexture* maybe_target)
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
        // then write linear color values to the render target, but
        // because we are *also* enabling GL_FRAMEBUFFER_SRGB, the OpenGL
        // backend will correctly convert those linear colors to sRGB if
        // necessary automatically
        convert_draw_data_from_srgb_to_linear(draw_list);

        Mesh& mesh = bd.mesh;
        mesh.clear();
        mesh.set_vertex_buffer_params(draw_list.VtxBuffer.Size, {
            {VertexAttribute::Position,  VertexAttributeFormat::Float32x2},
            {VertexAttribute::TexCoord0, VertexAttributeFormat::Float32x2},
            {VertexAttribute::Color,     VertexAttributeFormat::Unorm8x4},
        });
        mesh.set_vertex_buffer_data(std::span<ImDrawVert>{draw_list.VtxBuffer.Data, static_cast<size_t>(draw_list.VtxBuffer.Size)});
        mesh.set_indices({draw_list.IdxBuffer.Data, static_cast<size_t>(draw_list.IdxBuffer.size())}, {MeshUpdateFlag::DontRecalculateBounds, MeshUpdateFlag::DontValidateIndices});

        // iterate through command buffer
        for (const ImDrawCmd& draw_command : draw_list.CmdBuffer) {
            render_draw_command(bd, draw_data, draw_list, mesh, draw_command, maybe_target);
        }
        mesh.clear();
    }

    template<SameAsAnyOf<Texture2D, RenderTexture> Texture>
    ImTextureID allocate_texture_for_current_frame(const Texture& texture)
    {
        UiGraphicsContextData* bd = get_graphics_backend_data();
        OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error");
        const UID texture_uid = bd->textures_allocated_this_frame.try_emplace(UID{}, texture).first->first;
        return to_imgui_texture_id(texture_uid);
    }

    template<typename>
    ImGuiDataType data_type_for();

    template<>
    constexpr ImGuiDataType data_type_for<size_t>()
    {
        static_assert(std::is_unsigned_v<size_t>);
        static_assert(sizeof(size_t) == 8 or sizeof(size_t) == 4);
        return sizeof(size_t) == 8 ? ImGuiDataType_U64 : ImGuiDataType_U32;
    }
}

namespace
{
    void graphics_backend_init()
    {
        ImGuiIO& io = ImGui::GetIO();
        OSC_ASSERT(io.BackendRendererUserData == nullptr && "an oscar ImGui renderer backend is already initialized - this is a developer error (double-initialization)");

        // init backend data
        io.BackendRendererUserData = static_cast<void*>(new UiGraphicsContextData{});
        io.BackendRendererName = "imgui_impl_osc";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    }

    void graphics_backend_shutdown()
    {
        UiGraphicsContextData* bd = get_graphics_backend_data();
        OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error (double-free)");

        // shutdown platform interface
        ImGui::DestroyPlatformWindows();

        // destroy backend data
        ImGuiIO& io = ImGui::GetIO();
        io.BackendRendererName = nullptr;
        io.BackendRendererUserData = nullptr;
        delete bd;  // NOLINT(cppcoreguidelines-owning-memory)
    }

    void graphics_backend_on_start_new_frame()
    {
        // `ImGui_ImplOpenGL3_CreateDeviceObjects` is now part of constructing `OscarImguiBackendData`

        UiGraphicsContextData* bd = get_graphics_backend_data();
        OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available - this is a developer error");
        bd->textures_allocated_this_frame.clear();
        if (not bd->font_texture) {
            bd->font_texture = create_font_texture(bd->font_texture_id);
        }
        bd->textures_allocated_this_frame.try_emplace(bd->font_texture_id, *bd->font_texture);  // (so that all lookups can hit the same LUT)
    }

    void graphics_backend_mark_fonts_for_reupload()
    {
        if (UiGraphicsContextData* bd = get_graphics_backend_data()) {
            bd->font_texture.reset();
        }
    }

    void graphics_backend_render(ImDrawData* draw_data, RenderTexture* maybe_target = nullptr)
    {
        UiGraphicsContextData* bd = get_graphics_backend_data();
        OSC_ASSERT(bd != nullptr && "no oscar ImGui renderer backend was available to shutdown - this is a developer error");

        setup_camera_view_matrix(*draw_data, bd->camera);
        for (int n = 0; n < draw_data->CmdListsCount; ++n) {
            render_drawlist(*bd, *draw_data, *draw_data->CmdLists[n], maybe_target);
        }
    }

    ImTextureID graphics_backend_allocate_texture_for_current_frame(const Texture2D& texture)
    {
        return ::allocate_texture_for_current_frame(texture);
    }

    ImTextureID graphics_backend_allocate_texture_for_current_frame(const RenderTexture& texture)
    {
        return ::allocate_texture_for_current_frame(texture);
    }
}

namespace
{
    inline constexpr float c_default_drag_threshold = 5.0f;

    template<
        rgs::random_access_range TCollection,
        rgs::random_access_range UCollection
    >
    requires
        std::convertible_to<typename TCollection::value_type, float> and
        std::convertible_to<typename UCollection::value_type, float>
    float diff(const TCollection& older, const UCollection& newer, size_t n)
    {
        for (size_t i = 0; i < n; ++i) {
            if (static_cast<float>(older[i]) != static_cast<float>(newer[i])) {
                return newer[i];
            }
        }
        return static_cast<float>(older[0]);
    }

    Vec2 centroid_of(const ImRect& r)
    {
        return 0.5f * (Vec2{r.Min} + Vec2{r.Max});
    }

    Vec2 dimensions_of(const ImRect& r)
    {
        return Vec2{r.Max} - Vec2{r.Min};
    }

    float shortest_edge_length_of(const ImRect& r)
    {
        const Vec2 dimensions = dimensions_of(r);
        return rgs::min(dimensions);
    }

    ImU32 to_ImU32(const Color& color)
    {
        return ImGui::ColorConvertFloat4ToU32(color);
    }

    Color to_color(ImU32 u32color)
    {
        return Color{Vec4{ImGui::ColorConvertU32ToFloat4(u32color)}};
    }

    ImU32 brighten(ImU32 color, float factor)
    {
        const Color srgb = to_color(color);
        const Color brightened = factor * srgb;
        const Color saturated = saturate(brightened);
        return to_ImU32(saturated);
    }

    // maps between ui:: flag types and imgui flag types
    template<FlagsEnum SourceFlagType, typename DestinationImGuiFlagsType>
    requires std::is_enum_v<DestinationImGuiFlagsType> or std::is_integral_v<DestinationImGuiFlagsType>
    class FlagMapper {
    public:
        constexpr FlagMapper(std::initializer_list<std::pair<SourceFlagType, DestinationImGuiFlagsType>> mappings)
        {
            if (mappings.size() != num_flags<SourceFlagType>()) {
                throw std::runtime_error{"invalid number of flags passed to a FlagMapper"};
            }

            for (const auto& [source_flag, destination_flag] : mappings) {
                const auto source_index = std::countr_zero(std::bit_floor(std::to_underlying(source_flag)));
                lut_[source_index] = destination_flag;
            }
        }

        constexpr DestinationImGuiFlagsType operator()(Flags<SourceFlagType> flags) const
        {
            static_assert(std::is_unsigned_v<std::underlying_type_t<SourceFlagType>>);

            DestinationImGuiFlagsType rv{};
            for (auto v = to_underlying(flags); v;) {
                const size_t index = std::countr_zero(v);
                rv |= lut_[index];
                v ^= 1<<index;
            }
            return rv;
        }
    private:
        std::array<DestinationImGuiFlagsType, num_flags<SourceFlagType>()> lut_{};
    };
}

class osc::ui::ContextConfiguration::Impl final {
public:
    struct MainFontConfig final {
        ResourcePath path;
    };

    struct IconFontConfig final {
        ResourcePath path;
        ClosedInterval<char16_t> codepoint_range;
    };

    struct CustomFontConfig final {
        MainFontConfig main_font;
        IconFontConfig icon_font;
    };

    Impl() = default;

    void set_base_imgui_ini_config_resource(ResourcePath path)
    {
        base_imgui_ini_config_ = std::move(path);
    }

    void set_main_font_as_standard_plus_icon_font(
        ResourcePath main_font_ttf_path,
        ResourcePath icon_font_ttf_path,
        ClosedInterval<char16_t> codepoint_range)
    {
        custom_font_config_ = CustomFontConfig{
            .main_font = {.path = std::move(main_font_ttf_path)},
            .icon_font = {.path = std::move(icon_font_ttf_path), .codepoint_range = codepoint_range},
        };
    }

    const ResourcePath* base_imgui_ini_config() const
    {
        return base_imgui_ini_config_ ? &base_imgui_ini_config_.value() : nullptr;
    }

    const MainFontConfig* main_font_config() const
    {
        return custom_font_config_ ? &custom_font_config_->main_font: nullptr;
    }

    const IconFontConfig* icon_font_config() const
    {
        return custom_font_config_ ? &custom_font_config_->icon_font: nullptr;
    }
private:
    std::optional<ResourcePath> base_imgui_ini_config_;
    std::optional<CustomFontConfig> custom_font_config_;
};

namespace
{
    // this is necessary because ImGui will take ownership and be responsible for
    // freeing the memory with `ImGui::MemFree`
    char* to_imgui_allocated_copy(std::span<const char> span)
    {
        auto* ptr = std::bit_cast<char*>(ImGui::MemAlloc(span.size_bytes()));
        rgs::copy(span, ptr);
        return ptr;
    }

    void add_resource_as_font(
        ResourceLoader& loader,
        const ImFontConfig& config,
        ImFontAtlas& atlas,
        const ResourcePath& path,
        const ImWchar* glyph_ranges = nullptr)
    {
        const std::string base_font_data = loader.slurp(path);
        const std::span<const char> data_including_nul_terminator{base_font_data.data(), base_font_data.size() + 1};

        atlas.AddFontFromMemoryTTF(
            to_imgui_allocated_copy(data_including_nul_terminator),
            static_cast<int>(data_including_nul_terminator.size()),
            config.SizePixels,
            &config,
            glyph_ranges
        );
    }

    // The internal backend data associated with one UI context.
    struct UiContextData final {

        explicit UiContextData(CopyOnUpdPtr<ui::ContextConfiguration::Impl> config, WindowID window_id) :
            CallerConfig{std::move(config)},
            Window{window_id}
        {}

        CopyOnUpdPtr<ui::ContextConfiguration::Impl>     CallerConfig;

        WindowID                                         Window;
        WindowID                                         ImeWindow;  // important: used for UI's textual inputs (e.g. `ImGui::InputText`)
        std::string                                      ClipboardText;
        bool                                             WantChangeDisplayScale = false;
        std::optional<AppClock::time_point>              LastFrameTime;

        // Mouse handling
        WindowID                                         MouseWindowID;
        std::optional<CursorShape>                       CurrentCustomCursor;
        int                                              MouseButtonsDown = 0;
        int                                              MouseLastLeaveFrame = 0;
    };

    // Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
    // FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
    // FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
    UiContextData* try_get_ui_backend_data(ImGuiContext* context)
    {
        return context ? static_cast<UiContextData*>(context->IO.BackendPlatformUserData) : nullptr;
    }

    UiContextData* try_get_ui_backend_data()
    {
        return try_get_ui_backend_data(ImGui::GetCurrentContext());
    }

    UiContextData& get_backend_data()
    {
        UiContextData* bd = try_get_ui_backend_data();
        IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplOscar_Init()?");
        return *bd;
    }

    const char* ui_get_clipboard_text(ImGuiContext* context)
    {
        UiContextData* bd = try_get_ui_backend_data(context);
        bd->ClipboardText = get_clipboard_text();
        return bd->ClipboardText.c_str();
    }

    void ui_set_clipboard_text(ImGuiContext*, const char* text)
    {
        set_clipboard_text(text);
    }

    void load_imgui_config(
        const std::filesystem::path& user_data_directory,
        ResourceLoader& loader,
        const ui::ContextConfiguration::Impl& config)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags = 0;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // tabbing, using arrows to move around
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // dockable panels
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // OSCAR DOESN'T ALLOW IMGUI MULTI VIEWPORT

        // make it so that windows can only ever be moved from the title bar
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        // load application-level ImGui settings, then the user one,
        // so that the user settings takes precedence
        {
            // Load the "base" config, which is the configuration that's loaded if the
            // user hasn't got a configuration.
            if (const auto* base_path = config.base_imgui_ini_config(); base_path and loader.resource_exists(*base_path)) {
                const std::string base_ini_data = loader.slurp(*base_path);
                ImGui::LoadIniSettingsFromMemory(base_ini_data.data(), base_ini_data.size());
            }

            // CARE: the reason this filepath is `static` is because ImGui requires that
            // the string outlives the ImGui context
            static const std::string s_user_imgui_ini_file_path = (user_data_directory / "imgui.ini").string();

            ImGui::LoadIniSettingsFromDisk(s_user_imgui_ini_file_path.c_str());
            io.IniFilename = s_user_imgui_ini_file_path.c_str();
        }
    }

    void setup_scaling_dependent_rendering_fonts_and_styling(
        App& app,
        const ui::ContextConfiguration::Impl& config)
    {
        ImGuiIO& io = ImGui::GetIO();
        const float scale = app.main_window_device_pixel_ratio();

        // Setup ImGui-to-renderer scaling for HighDPI support.
        io.DisplayFramebufferScale = {scale, scale};

        // Setup fonts: ensure they have they have the correct pixel scaling for HighDPI.
        {
            ImFontConfig base_font_config;
            base_font_config.SizePixels = c_default_base_font_pixel_size;
            base_font_config.RasterizerDensity = scale;
            base_font_config.PixelSnapH = true;
            base_font_config.FontDataOwnedByAtlas = true;

            ResourceLoader loader = app.upd_resource_loader();
            bool should_build_and_reupload = false;

            // Main font support
            if (const auto* main_font = config.main_font_config(); main_font and loader.resource_exists(main_font->path)) {
                io.Fonts->Clear();
                io.FontDefault = nullptr;

                add_resource_as_font(loader, base_font_config, *io.Fonts, main_font->path);
                should_build_and_reupload = true;
            }

            // Add icon support
            if (const auto* icon_font = config.icon_font_config(); should_build_and_reupload and icon_font and loader.resource_exists(icon_font->path)) {
                ImFontConfig icon_font_config = base_font_config;
                icon_font_config.MergeMode = true;
                icon_font_config.GlyphMinAdvanceX = floor(1.5f * icon_font_config.SizePixels);
                icon_font_config.GlyphMaxAdvanceX = floor(1.5f * icon_font_config.SizePixels);
                static_assert(sizeof(decltype(icon_font->codepoint_range.lower)) == sizeof(ImWchar));
                const auto c_icon_ranges = std::to_array<ImWchar>({icon_font->codepoint_range.lower, icon_font->codepoint_range.upper, 0 });

                add_resource_as_font(loader, icon_font_config, *io.Fonts, icon_font->path, c_icon_ranges.data());
                should_build_and_reupload = true;
            }

            if (should_build_and_reupload) {
                io.Fonts->Build();
                graphics_backend_mark_fonts_for_reupload();
            }
        }

        // Setup visual styling/theme.
        {
            ImGui::GetStyle() = ImGuiStyle{};
            ui::apply_dark_theme();
        }
    }

    // note: the native IME UI will only display if user calls `SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1")`
    //       before `SDL_CreateWindow`.
    //
    //       However, even if the native overlay isn't showing it's still __VERY IMPORTANT__ to handle
    //       IME correctly, because ImGui's text input widgets use text input events, rather than key
    //       events, to track user input.
    void ImGui_ImplOscar_PlatformSetImeData(
        ImGuiContext*,
        ImGuiViewport* viewport,
        ImGuiPlatformImeData* ime_data)
    {
        App& app = App::upd();
        UiContextData* bd = try_get_ui_backend_data();
        WindowID viewport_window{viewport->PlatformHandle};

        if (bd->ImeWindow and (not ime_data->WantVisible or bd->ImeWindow != viewport_window)) {
            app.stop_text_input(std::exchange(bd->ImeWindow, WindowID{}));
        }

        if (ime_data->WantVisible) {
            const Vec2 input_dimensions = {1.0f, ime_data->InputLineHeight};
            const Vec2 input_top_left_ui = to<Vec2>(ime_data->InputPos);
            const Vec2 input_bottom_left_ui = {input_top_left_ui.x, input_top_left_ui.y + input_dimensions.y};
            const Vec2 input_bottom_left_screen = {input_top_left_ui.x, viewport->Size.y - input_bottom_left_ui.y};

            app.set_main_window_unicode_input_rect(Rect{input_bottom_left_screen, input_bottom_left_screen + input_dimensions});
            app.start_text_input(bd->Window);
            bd->ImeWindow = viewport_window;
        }
    }

    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    // If you have multiple SDL events and some of them are not meant to be used by dear imgui, you may need to filter events based on their windowID field.
    bool ImGui_ImplOscar_ProcessEvent(Event& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        UiContextData* bd = try_get_ui_backend_data();

        switch (e.type()) {
        case EventType::MouseMove: {
            const auto& move_event = dynamic_cast<const MouseEvent&>(e);
            io.AddMouseSourceEvent(move_event.input_source() == MouseInputSource::TouchScreen ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
            io.AddMousePosEvent(move_event.location().x, io.DisplaySize.y - move_event.location().y);
            return true;
        }
        case EventType::MouseWheel: {
            const auto& wheel_event = dynamic_cast<const MouseWheelEvent&>(e);
            auto [x, y] = wheel_event.delta();
            io.AddMouseSourceEvent(wheel_event.input_source() == MouseInputSource::TouchScreen ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
            io.AddMouseWheelEvent(x, y);
            return true;
        }
        case EventType::MouseButtonDown:
        case EventType::MouseButtonUp: {
            const auto& button_event = dynamic_cast<const MouseEvent&>(e);
            const auto button = button_event.button();

            int mouse_button = -1;
            if (button == MouseButton::Left)    { mouse_button = 0; }
            if (button == MouseButton::Right)   { mouse_button = 1; }
            if (button == MouseButton::Middle)  { mouse_button = 2; }
            if (button == MouseButton::Back)    { mouse_button = 3; }
            if (button == MouseButton::Forward) { mouse_button = 4; }

            if (mouse_button == -1) {
                return false;
            }

            io.AddMouseSourceEvent(button_event.input_source() == MouseInputSource::TouchScreen ? ImGuiMouseSource_TouchScreen : ImGuiMouseSource_Mouse);
            io.AddMouseButtonEvent(mouse_button, button_event.type() == EventType::MouseButtonDown);
            bd->MouseButtonsDown = (button_event.type() == EventType::MouseButtonDown) ? (bd->MouseButtonsDown | (1 << mouse_button)) : (bd->MouseButtonsDown & ~(1 << mouse_button));
            return true;
        }
        case EventType::KeyDown:
        case EventType::KeyUp: {
            const auto& key_event = dynamic_cast<const KeyEvent&>(e);

            io.AddKeyEvent(ImGuiMod_Ctrl,  key_event.has_modifier(KeyModifier::Ctrl));
            io.AddKeyEvent(ImGuiMod_Shift, key_event.has_modifier(KeyModifier::Shift));
            io.AddKeyEvent(ImGuiMod_Alt,   key_event.has_modifier(KeyModifier::Alt));
            io.AddKeyEvent(ImGuiMod_Super, key_event.has_modifier(KeyModifier::Meta));

            io.AddKeyEvent(to<ImGuiKey>(key_event.key()), key_event.type() == EventType::KeyDown);
            return true;
        }
        case EventType::TextInput: {
            const auto& text_event = dynamic_cast<const TextInputEvent&>(e);
            io.AddInputCharactersUTF8(text_event.utf8_text().c_str());
            return true;
        }
        case EventType::DisplayStateChange: {
            // 2.0.26 has SDL_DISPLAYEVENT_CONNECTED/SDL_DISPLAYEVENT_DISCONNECTED/SDL_DISPLAYEVENT_ORIENTATION,
            // so change of DPI/Scaling are not reflected in this event. (SDL3 has it)

            // triggered when monitors are connected/disconnected: oscar doesn't use imgui's multi-viewport
            // so we don't need this
            return true;
        }
        case EventType::Window: {
            // - When capturing mouse, SDL will send a bunch of conflicting LEAVE/ENTER event on every mouse move, but the final ENTER tends to be right.
            // - However we won't get a correct LEAVE event for a captured window.
            // - In some cases, when detaching a window from main viewport SDL may send SDL_WINDOWEVENT_ENTER one frame too late,
            //   causing SDL_WINDOWEVENT_LEAVE on previous frame to interrupt drag operation by clear mouse position. This is why
            //   we delay process the SDL_WINDOWEVENT_LEAVE events by one frame. See issue #5012 for details.
            const auto& window_event = dynamic_cast<const WindowEvent&>(e);

            switch (window_event.type()) {
            case WindowEventType::GainedMouseFocus: {
                bd->MouseWindowID = window_event.window();
                bd->MouseLastLeaveFrame = 0;
                return true;
            }
            case WindowEventType::LostMouseFocus: {
                bd->MouseLastLeaveFrame = ImGui::GetFrameCount() + 1;
                return true;
            }
            case WindowEventType::GainedKeyboardFocus: {
                io.AddFocusEvent(true);
                return true;
            }
            case WindowEventType::LostKeyboardFocus: {
                io.AddFocusEvent(false);
                return true;
            }
            case WindowEventType::WindowClosed: {
                ImGuiViewport* vp = ImGui::GetMainViewport();
                if (window_event.window() == WindowID{vp->PlatformHandle}) {
                    vp->PlatformRequestClose = true;
                }
                return true;
            }
            case WindowEventType::WindowMoved: {
                ImGuiViewport* vp = ImGui::GetMainViewport();
                if (window_event.window() == WindowID{vp->PlatformHandle}) {
                    vp->PlatformRequestMove = true;
                }
                return true;
            }
            case WindowEventType::WindowResized: {
                ImGuiViewport* vp = ImGui::GetMainViewport();
                if (window_event.window() == WindowID{vp->PlatformHandle}) {
                    vp->PlatformRequestResize = true;
                }
                return true;
            }
            case WindowEventType::WindowDisplayScaleChanged: {
                bd->WantChangeDisplayScale = true;
                return true;
            }
            default: {
                return true;
            }
            }
        }
        default: {
            return false;
        }
        }
    }

    void ImGui_ImplOscar_Init(CopyOnUpdPtr<ui::ContextConfiguration::Impl> config, WindowID main_window_id)
    {
        ImGuiIO& io = ImGui::GetIO();
        OSC_ASSERT_ALWAYS(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

        // init `BackendData` and setup `ImGuiIO` pointers etc.
        io.BackendPlatformUserData = static_cast<void*>(new UiContextData{std::move(config), main_window_id});
        io.BackendPlatformName = "imgui_impl_oscar";
        io.BackendFlags = ImGuiBackendFlags_None;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;  // We can honor GetMouseCursor() values (optional)
        io.ConfigDebugHighlightIdConflicts = false;            // Disable this highlight (annoying for users, #964)

        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        platform_io.Platform_SetClipboardTextFn = ui_set_clipboard_text;
        platform_io.Platform_GetClipboardTextFn = ui_get_clipboard_text;
        platform_io.Platform_ClipboardUserData = nullptr;
        platform_io.Platform_SetImeDataFn = ImGui_ImplOscar_PlatformSetImeData;
        platform_io.Platform_OpenInShellFn = [](ImGuiContext*, const char* url)
        {
            osc::open_url_in_os_default_web_browser(url);
            return true;
        };

        // init `ImGuiViewport` for main viewport
        //
        // Our mouse update function expect PlatformHandle to be filled for the main viewport
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->PlatformHandle = to<void*>(main_window_id);
        main_viewport->PlatformHandleRaw = nullptr;  // oscar: don't expose underlying OS/App abstraction
    }

    void ImGui_ImplOscar_Shutdown(App& app)
    {
        UiContextData* bd = try_get_ui_backend_data();
        OSC_ASSERT_ALWAYS(bd != nullptr && "No platform backend to shutdown, or already shutdown?");

        if (bd->CurrentCustomCursor) {
            app.pop_cursor_override();
        }

        delete bd;  // NOLINT(cppcoreguidelines-owning-memory)

        ImGuiIO& io = ImGui::GetIO();
        io.BackendPlatformName = nullptr;
        io.BackendPlatformUserData = nullptr;
        io.BackendFlags = ImGuiBackendFlags_None;
    }

    void ImGui_ImplOscar_UpdateMouseCursor(App& app)
    {
        const ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) {
            return;  // ui cannot change the mouse cursor
        }

        UiContextData& bd = get_backend_data();
        const auto oscar_cursor = to<CursorShape>(ImGui::GetMouseCursor());

        if (oscar_cursor != bd.CurrentCustomCursor) {
            if (bd.CurrentCustomCursor) {
                app.pop_cursor_override();
            }
            app.push_cursor_override(Cursor{oscar_cursor});
            bd.CurrentCustomCursor = oscar_cursor;
        }
    }

    void ImGui_ImplOscar_NewFrame(App& app)
    {
        UiContextData& bd = get_backend_data();
        ImGuiIO& io = ImGui::GetIO();

        // Setup `DisplaySize` and `DisplayFramebufferScale`
        //
        // Performed every frame to accommodate for runtime window resizes
        {
            auto window_dimensions = app.main_window_dimensions();
            if (app.is_main_window_minimized()) {
                window_dimensions = {};
            }
            io.DisplaySize = to<ImVec2>(window_dimensions);
        }

        // Update display scale (e.g. when user changes DPI settings or moves the
        // application window to a display that has a different DPI)
        if (std::exchange(bd.WantChangeDisplayScale, false)) {
            setup_scaling_dependent_rendering_fonts_and_styling(app, *bd.CallerConfig);
        }

        // Update `DeltaTime`
        {
            auto t = app.frame_start_time();  // note: might not increase (#935)
            if (bd.LastFrameTime and t <= *bd.LastFrameTime) {
                // handle the case where the clock hasn't increased since the last frame by
                // adding a very small amount of time, because ImGui doesn't accept a `DeltaTime`
                // of zero (see: imgui/#6189, imgui/#6114, imgui/#3644)
                static_assert(static_cast<float>(std::chrono::duration<AppClock::rep, std::nano>{1}.count()) > std::numeric_limits<float>::epsilon());
                t = *bd.LastFrameTime + std::chrono::nanoseconds{1};
            }
            const auto delta = bd.LastFrameTime ? t - *bd.LastFrameTime : AppClock::duration{1.0/60.0};
            io.DeltaTime = static_cast<float>(delta.count());
            bd.LastFrameTime = t;

        }

        // Handle mouse leaving the window
        if (bd.MouseLastLeaveFrame and (bd.MouseLastLeaveFrame >= ImGui::GetFrameCount()) and bd.MouseButtonsDown == 0) {
            bd.MouseWindowID.reset();
            bd.MouseLastLeaveFrame = 0;
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        }

        // update mouse position
        if (const auto p = App::upd().mouse_pos_in_main_window()) {
            ImGui::GetIO().AddMousePosEvent(p->x, io.DisplaySize.y - p->y);
        }
        ImGui_ImplOscar_UpdateMouseCursor(app);
    }

    constexpr auto c_combo_lut = std::to_array<std::pair<PhysicalKeyModifier, std::string_view>>({
        {PhysicalKeyModifier::Shift, "Shift "},
        {PhysicalKeyModifier::Ctrl, "Ctrl "},
        {PhysicalKeyModifier::Meta, "Command "},  // TODO
        {PhysicalKeyModifier::Alt, "Alt "},
    });

    std::string_view to_human_readable(Key key)
    {
        static_assert(num_options<Key>() == 120);
        switch (key) {
        case Key::Tab: return "Tab";
        case Key::LeftArrow: return "LeftArrow";
        case Key::RightArrow: return "RightArrow";
        case Key::UpArrow: return "UpArrow";
        case Key::DownArrow: return "DownArrow";
        case Key::PageUp: return "PageUp";
        case Key::PageDown: return "PageDown";
        case Key::Home: return "Home";
        case Key::End: return "End";
        case Key::Insert: return "Insert";
        case Key::Delete: return "Delete";
        case Key::Backspace: return "Backspace";
        case Key::Space: return "Space";
        case Key::Return: return "Return";
        case Key::Escape: return "Escape";
        case Key::Apostrophe: return "Apostrophe";
        case Key::Comma: return "Comma";
        case Key::Minus: return "Minus";
        case Key::Period: return "Period";
        case Key::Slash: return "Slash";
        case Key::Semicolon: return "Semicolon";
        case Key::Equals: return "Equals";
        case Key::LeftBracket: return "LeftBracket";
        case Key::Backslash: return "Backslash";
        case Key::RightBracket: return "RightBracket";
        case Key::Grave:  // backquote return "Grave";
        case Key::CapsLock: return "CapsLock";
        case Key::ScrollLock: return "ScrollLock";
        case Key::NumLockClear: return "NumLockClear";
        case Key::PrintScreen: return "PrintScreen";
        case Key::Pause: return "Pause";
        case Key::Keypad0: return "Keypad0";
        case Key::Keypad1: return "Keypad1";
        case Key::Keypad2: return "Keypad2";
        case Key::Keypad3: return "Keypad3";
        case Key::Keypad4: return "Keypad4";
        case Key::Keypad5: return "Keypad5";
        case Key::Keypad6: return "Keypad6";
        case Key::Keypad7: return "Keypad7";
        case Key::Keypad8: return "Keypad8";
        case Key::Keypad9: return "Keypad9";
        case Key::KeypadPeriod: return "KeypadPeriod";
        case Key::KeypadDivide: return "KeypadDivide";
        case Key::KeypadMultiply: return "KeypadMultiply";
        case Key::KeypadMinus: return "KeypadMinus";
        case Key::KeypadPlus: return "KeypadPlus";
        case Key::KeypadEnter: return "KeypadEnter";
        case Key::KeypadEquals: return "KeypadEquals";
        case Key::LeftCtrl: return "LeftCtrl";
        case Key::LeftShift: return "LeftShift";
        case Key::LeftAlt: return "LeftAlt";
        case Key::LeftGui: return "LeftGui";
        case Key::RightCtrl: return "RightCtrl";
        case Key::RightShift: return "RightShift";
        case Key::RightAlt: return "RightAlt";
        case Key::RightGui: return "RightGui";
        case Key::Application: return "Application";
        case Key::_0: return "0";
        case Key::_1: return "1";
        case Key::_2: return "2";
        case Key::_3: return "3";
        case Key::_4: return "4";
        case Key::_5: return "5";
        case Key::_6: return "6";
        case Key::_7: return "7";
        case Key::_8: return "8";
        case Key::_9: return "9";
        case Key::A: return "A";
        case Key::B: return "B";
        case Key::C: return "C";
        case Key::D: return "D";
        case Key::E: return "E";
        case Key::F: return "F";
        case Key::G: return "G";
        case Key::H: return "H";
        case Key::I: return "I";
        case Key::J: return "J";
        case Key::K: return "K";
        case Key::L: return "L";
        case Key::M: return "M";
        case Key::N: return "N";
        case Key::O: return "O";
        case Key::P: return "P";
        case Key::Q: return "Q";
        case Key::R: return "R";
        case Key::S: return "S";
        case Key::T: return "T";
        case Key::U: return "U";
        case Key::V: return "V";
        case Key::W: return "W";
        case Key::X: return "X";
        case Key::Y: return "Y";
        case Key::Z: return "Z";
        case Key::F1: return "F1";
        case Key::F2: return "F2";
        case Key::F3: return "F3";
        case Key::F4: return "F4";
        case Key::F5: return "F5";
        case Key::F6: return "F6";
        case Key::F7: return "F7";
        case Key::F8: return "F8";
        case Key::F9: return "F9";
        case Key::F10: return "F10";
        case Key::F11: return "F11";
        case Key::F12: return "F12";
        case Key::F13: return "F13";
        case Key::F14: return "F14";
        case Key::F15: return "F15";
        case Key::F16: return "F16";
        case Key::F17: return "F17";
        case Key::F18: return "F18";
        case Key::F19: return "F19";
        case Key::F20: return "F20";
        case Key::F21: return "F21";
        case Key::F22: return "F22";
        case Key::F23: return "F23";
        case Key::F24: return "F24";
        case Key::AppBack: return "AppBack";
        case Key::AppForward: return "AppForward";
        default:
        case Key::Unknown: return "Unknown";
        }
    }

    std::string to_human_readable_representation(KeyCombination shortcut)
    {
        const auto user_modifiers = to<PhysicalKeyModifiers>(shortcut.modifiers());

        std::stringstream ss;
        for (const auto& [mod, label] : c_combo_lut) {
            if (mod & user_modifiers) {
                ss << label;
            }
        }
        ss << to_human_readable(shortcut.key());
        return std::move(ss).str();
    }
}

template<>
struct osc::Converter<ui::GizmoOperation, ImGuizmo::OPERATION> final {
    ImGuizmo::OPERATION operator()(ui::GizmoOperation op) const
    {
        static_assert(num_flags<ui::GizmoOperation>() == 3);
        switch (op) {
        case ui::GizmoOperation::Scale:     return ImGuizmo::OPERATION::SCALE;
        case ui::GizmoOperation::Rotate:    return ImGuizmo::OPERATION::ROTATE;
        case ui::GizmoOperation::Translate: return ImGuizmo::OPERATION::TRANSLATE;
        default:                            return ImGuizmo::OPERATION::TRANSLATE;
        }
    }
};

template<>
struct osc::Converter<ui::GizmoMode, ImGuizmo::MODE> final {
    ImGuizmo::MODE operator()(ui::GizmoMode mode) const
    {
        static_assert(num_options<ui::GizmoMode>() == 2);
        switch (mode) {
        case ui::GizmoMode::Local: return ImGuizmo::MODE::LOCAL;
        case ui::GizmoMode::World: return ImGuizmo::MODE::WORLD;
        default:                   return ImGuizmo::MODE::WORLD;
        }
    }
};

template<>
struct osc::Converter<ui::TreeNodeFlags, ImGuiTreeNodeFlags> final {
    ImGuiTreeNodeFlags operator()(ui::TreeNodeFlags flags) const { return c_mappings_(flags); }
private:
    static constexpr FlagMapper<ui::TreeNodeFlag, ImGuiTreeNodeFlags> c_mappings_ = {
        {ui::TreeNodeFlag::DefaultOpen, ImGuiTreeNodeFlags_DefaultOpen},
        {ui::TreeNodeFlag::OpenOnArrow, ImGuiTreeNodeFlags_OpenOnArrow},
        {ui::TreeNodeFlag::Leaf,        ImGuiTreeNodeFlags_Leaf},
        {ui::TreeNodeFlag::Bullet,      ImGuiTreeNodeFlags_Bullet},
    };
};

template<>
struct osc::Converter<ui::TabItemFlags, ImGuiTabItemFlags> final {
    ImGuiTabItemFlags operator()(ui::TabItemFlags flags) const { return c_mappings_(flags); }
private:
    static constexpr FlagMapper<ui::TabItemFlag, ImGuiTabItemFlags> c_mappings_ = {
        {ui::TabItemFlag::NoReorder,       ImGuiTabItemFlags_NoReorder},
        {ui::TabItemFlag::NoCloseButton,   ImGuiTabItemFlags_NoCloseButton},
        {ui::TabItemFlag::UnsavedDocument, ImGuiTabItemFlags_UnsavedDocument},
        {ui::TabItemFlag::SetSelected,     ImGuiTabItemFlags_SetSelected},
    };
};

template<>
struct osc::Converter<ui::MouseButton, ImGuiMouseButton> final {
    ImGuiMouseButton operator()(ui::MouseButton button) const
    {
        static_assert(ImGuiMouseButton_COUNT == 5);
        static_assert(num_options<ui::MouseButton>() == 3);

        switch (button) {
        case ui::MouseButton::Left:   return ImGuiMouseButton_Left;
        case ui::MouseButton::Right:  return ImGuiMouseButton_Right;
        case ui::MouseButton::Middle: return ImGuiMouseButton_Middle;
        default:                      return ImGuiMouseButton_Left;  // shouldn't happen
        }
    }
};

template<>
struct osc::Converter<ui::SliderFlags, ImGuiSliderFlags> final {
    ImGuiSliderFlags operator()(ui::SliderFlags flags) const
    {
        return c_mappings_(flags);
    }
private:
    static constexpr FlagMapper<ui::SliderFlag, ImGuiSliderFlags> c_mappings_ = {
        {ui::SliderFlag::Logarithmic, ImGuiSliderFlags_Logarithmic},
        {ui::SliderFlag::AlwaysClamp, ImGuiSliderFlags_AlwaysClamp},
        {ui::SliderFlag::NoInput,     ImGuiSliderFlags_NoInput},
    };
};

template<>
struct osc::Converter<ui::DataType, ImGuiDataType> final {
    ImGuiDataType operator()(ui::DataType) const
    {
        static_assert(num_options<ui::DataType>() == 1);
        return ImGuiDataType_Float;
    }
};

template<>
struct osc::Converter<ui::TextInputFlags, ImGuiInputTextFlags> final {
    ImGuiInputTextFlags operator()(ui::TextInputFlags flags) const
    {
        return c_mappings_(flags);
    }
private:
    static constexpr FlagMapper<ui::TextInputFlag, ImGuiInputTextFlags> c_mappings_ = {
        {ui::TextInputFlag::EnterReturnsTrue, ImGuiInputTextFlags_EnterReturnsTrue},
        {ui::TextInputFlag::ReadOnly,         ImGuiInputTextFlags_ReadOnly},
    };
};

template<>
struct osc::Converter<ui::ComboFlags, ImGuiComboFlags> final {
    ImGuiComboFlags operator()(ui::ComboFlags flags) const
    {
        return c_mappings_(flags);
    }
private:
    static constexpr FlagMapper<ui::ComboFlag, ImGuiComboFlags> c_mappings_ = {
        {ui::ComboFlag::NoArrowButton, ImGuiComboFlags_NoArrowButton},
    };
};

template<>
struct osc::Converter<ui::PanelFlags, ImGuiWindowFlags> final {
    ImGuiWindowFlags operator()(ui::PanelFlags flags) const
    {
        return c_mappings_(flags);
    }
private:
    static constexpr FlagMapper<ui::PanelFlag, ImGuiWindowFlags> c_mappings_ = {
        {ui::PanelFlag::NoMove                 , ImGuiWindowFlags_NoMove                 },
        {ui::PanelFlag::NoTitleBar             , ImGuiWindowFlags_NoTitleBar             },
        {ui::PanelFlag::NoResize               , ImGuiWindowFlags_NoResize               },
        {ui::PanelFlag::NoSavedSettings        , ImGuiWindowFlags_NoSavedSettings        },
        {ui::PanelFlag::NoScrollbar            , ImGuiWindowFlags_NoScrollbar            },
        {ui::PanelFlag::NoInputs               , ImGuiWindowFlags_NoInputs               },
        {ui::PanelFlag::NoBackground           , ImGuiWindowFlags_NoBackground           },
        {ui::PanelFlag::NoCollapse             , ImGuiWindowFlags_NoCollapse             },
        {ui::PanelFlag::NoDecoration           , ImGuiWindowFlags_NoDecoration           },
        {ui::PanelFlag::NoDocking              , ImGuiWindowFlags_NoDocking              },

        {ui::PanelFlag::NoNav                  , ImGuiWindowFlags_NoNav                  },
        {ui::PanelFlag::MenuBar                , ImGuiWindowFlags_MenuBar                },
        {ui::PanelFlag::AlwaysAutoResize       , ImGuiWindowFlags_AlwaysAutoResize       },
        {ui::PanelFlag::HorizontalScrollbar    , ImGuiWindowFlags_HorizontalScrollbar    },
        {ui::PanelFlag::AlwaysVerticalScrollbar, ImGuiWindowFlags_AlwaysVerticalScrollbar},
    };
};

template<>
struct osc::Converter<ui::ChildPanelFlags, ImGuiChildFlags> final {
    ImGuiChildFlags operator()(ui::ChildPanelFlags flags) const
    {
        return c_mappings_(flags);
    }
private:
    static constexpr FlagMapper<ui::ChildPanelFlag, ImGuiChildFlags> c_mappings_ = {
        {ui::ChildPanelFlag::Border, ImGuiChildFlags_Border},
    };
};

template<>
struct osc::Converter<ui::Conditional, ImGuiCond> final {
    ImGuiCond operator()(ui::Conditional conditional) const
    {
        static_assert(num_options<ui::Conditional>() == 3);

        switch (conditional) {
        case ui::Conditional::Always:    return ImGuiCond_Always;
        case ui::Conditional::Once:      return ImGuiCond_Once;
        case ui::Conditional::Appearing: return ImGuiCond_Appearing;
        default:                         return ImGuiCond_Always;
        }
    }
};

template<>
struct osc::Converter<ui::HoveredFlags, ImGuiHoveredFlags> final {
    ImGuiHoveredFlags operator()(ui::HoveredFlags flags) const
    {
        return c_mappings_(flags);
    }
private:
    static constexpr FlagMapper<ui::HoveredFlag, ImGuiHoveredFlags> c_mappings_ = {
        {ui::HoveredFlag::AllowWhenDisabled           , ImGuiHoveredFlags_AllowWhenDisabled           },
        {ui::HoveredFlag::AllowWhenBlockedByPopup     , ImGuiHoveredFlags_AllowWhenBlockedByPopup     },
        {ui::HoveredFlag::AllowWhenBlockedByActiveItem, ImGuiHoveredFlags_AllowWhenBlockedByActiveItem},
        {ui::HoveredFlag::AllowWhenOverlapped         , ImGuiHoveredFlags_AllowWhenOverlapped         },
        {ui::HoveredFlag::DelayNormal                 , ImGuiHoveredFlags_DelayNormal                 },
        {ui::HoveredFlag::ForTooltip                  , ImGuiHoveredFlags_ForTooltip                  },
        {ui::HoveredFlag::RootAndChildPanels          , ImGuiHoveredFlags_RootAndChildWindows         },
        {ui::HoveredFlag::ChildPanels                 , ImGuiHoveredFlags_ChildWindows                },
    };
};

template<>
struct osc::Converter<ui::ItemFlags, ImGuiItemFlags> final {
    ImGuiItemFlags operator()(ui::ItemFlags flags) const
    {
        return c_mappings_(flags);
    }
private:
    static constexpr FlagMapper<ui::ItemFlag, ImGuiItemFlags> c_mappings_ = {
        {ui::ItemFlag::Disabled , ImGuiItemFlags_Disabled },
        {ui::ItemFlag::Inputable, ImGuiItemFlags_Inputable},
    };
};

template<>
struct osc::Converter<ui::PopupFlags, ImGuiPopupFlags> final {
    ImGuiPopupFlags operator()(ui::PopupFlags flags) const
    {
        return c_mappings_(flags);
    }
private:
    static constexpr FlagMapper<ui::PopupFlag, ImGuiPopupFlags> c_mappings_ = {
        {ui::PopupFlag::MouseButtonLeft , ImGuiPopupFlags_MouseButtonLeft },
        {ui::PopupFlag::MouseButtonRight, ImGuiPopupFlags_MouseButtonRight},
    };
};

template<>
struct osc::Converter<ui::TableFlags, ImGuiTableFlags> final {
    ImGuiTableFlags operator()(ui::TableFlags flags) const
    {
        return c_mappings_(flags);
    }
private:
    static constexpr FlagMapper<ui::TableFlag, ImGuiTableFlags> c_mappings_ = {
        {ui::TableFlag::BordersInner     , ImGuiTableFlags_BordersInner     },
        {ui::TableFlag::BordersInnerV    , ImGuiTableFlags_BordersInnerV    },
        {ui::TableFlag::NoSavedSettings  , ImGuiTableFlags_NoSavedSettings  },
        {ui::TableFlag::PadOuterX        , ImGuiTableFlags_PadOuterX        },
        {ui::TableFlag::Resizable        , ImGuiTableFlags_Resizable        },
        {ui::TableFlag::ScrollY          , ImGuiTableFlags_ScrollY          },
        {ui::TableFlag::SizingStretchProp, ImGuiTableFlags_SizingStretchProp},
        {ui::TableFlag::SizingStretchSame, ImGuiTableFlags_SizingStretchSame},
        {ui::TableFlag::Sortable         , ImGuiTableFlags_Sortable         },
        {ui::TableFlag::SortTristate     , ImGuiTableFlags_SortTristate     },
    };
};

template<>
struct osc::Converter<ui::ColumnFlags, ImGuiTableColumnFlags> final {
    ImGuiTableColumnFlags operator()(ui::ColumnFlags flags) const
    {
        return c_mappings_(flags);
    }
private:
    static constexpr FlagMapper<ui::ColumnFlag, ImGuiTableColumnFlags> c_mappings_ = {
        {ui::ColumnFlag::NoSort,       ImGuiTableColumnFlags_NoSort},
        {ui::ColumnFlag::WidthStretch, ImGuiTableColumnFlags_WidthStretch},
    };
};

template<>
struct osc::Converter<ui::ColorVar, ImGuiCol> final {
    ImGuiCol operator()(ui::ColorVar option) const
    {
        static_assert(num_options<ui::ColorVar>() == 11);

        switch (option) {
        case ui::ColorVar::Text:           return ImGuiCol_Text;
        case ui::ColorVar::Button:         return ImGuiCol_Button;
        case ui::ColorVar::ButtonActive:   return ImGuiCol_ButtonActive;
        case ui::ColorVar::ButtonHovered:  return ImGuiCol_ButtonHovered;
        case ui::ColorVar::FrameBg:        return ImGuiCol_FrameBg;
        case ui::ColorVar::PopupBg:        return ImGuiCol_PopupBg;
        case ui::ColorVar::FrameBgHovered: return ImGuiCol_FrameBgHovered;
        case ui::ColorVar::FrameBgActive:  return ImGuiCol_FrameBgActive;
        case ui::ColorVar::CheckMark:      return ImGuiCol_CheckMark;
        case ui::ColorVar::SliderGrab:     return ImGuiCol_SliderGrab;
        case ui::ColorVar::PanelBg:        return ImGuiCol_WindowBg;
        default:                           return ImGuiCol_Text;
        }
    }
};

template<>
struct osc::Converter<ui::StyleVar, ImGuiStyleVar> final {
    ImGuiStyleVar operator()(ui::StyleVar option) const
    {
        static_assert(num_options<ui::StyleVar>() == 10);

        switch (option) {
        case ui::StyleVar::Alpha:            return ImGuiStyleVar_Alpha;
        case ui::StyleVar::ButtonTextAlign:  return ImGuiStyleVar_ButtonTextAlign;
        case ui::StyleVar::CellPadding:      return ImGuiStyleVar_CellPadding;
        case ui::StyleVar::DisabledAlpha:    return ImGuiStyleVar_DisabledAlpha;
        case ui::StyleVar::FramePadding:     return ImGuiStyleVar_FramePadding;
        case ui::StyleVar::FrameRounding:    return ImGuiStyleVar_FrameRounding;
        case ui::StyleVar::ItemInnerSpacing: return ImGuiStyleVar_ItemInnerSpacing;
        case ui::StyleVar::ItemSpacing:      return ImGuiStyleVar_ItemSpacing;
        case ui::StyleVar::TabRounding:      return ImGuiStyleVar_TabRounding;
        case ui::StyleVar::PanelPadding:     return ImGuiStyleVar_WindowPadding;
        default:                             return ImGuiStyleVar_Alpha;
        }
    }
};

template<>
struct osc::Converter<ImGuiSortDirection, ui::SortDirection> final {
    ui::SortDirection operator()(ImGuiSortDirection option) const
    {
        static_assert(num_options<ui::SortDirection>() == 3);

        switch (option) {
        case ImGuiSortDirection_None:       return ui::SortDirection::None;
        case ImGuiSortDirection_Ascending:  return ui::SortDirection::Ascending;
        case ImGuiSortDirection_Descending: return ui::SortDirection::Descending;
        default:                            return ui::SortDirection::None;
        }
    }
};


template<>
struct osc::Converter<ImGuiTableColumnSortSpecs, ui::TableColumnSortSpec> final {
    ui::TableColumnSortSpec operator()(ImGuiTableColumnSortSpecs specs) const
    {
        return {
            .column_id = ui::ID{specs.ColumnUserID},
            .column_index = static_cast<size_t>(specs.ColumnIndex),
            .sort_order = static_cast<size_t>(specs.SortOrder),
            .sort_direction = to<ui::SortDirection>(specs.SortDirection),
        };
    }
};

osc::ui::ContextConfiguration::ContextConfiguration() :
    impl_{make_cow<Impl>()}
{}

void osc::ui::ContextConfiguration::set_base_imgui_ini_config_resource(ResourcePath path)
{
    impl_.upd()->set_base_imgui_ini_config_resource(std::move(path));
}

void osc::ui::ContextConfiguration::set_main_font_as_standard_plus_icon_font(
    ResourcePath main_font_ttf_path,
    ResourcePath icon_font_ttf_path,
    ClosedInterval<char16_t> codepoint_range)
{
    impl_.upd()->set_main_font_as_standard_plus_icon_font(
        std::move(main_font_ttf_path),
        std::move(icon_font_ttf_path),
        codepoint_range
    );
}

osc::ui::Context::Context(App& app, ContextConfiguration configuration)
{
    init(app, std::move(configuration).impl());
}

osc::ui::Context::~Context() noexcept
{
    shutdown(App::upd());
}

void osc::ui::Context::reset()
{
    App& app = App::upd();
    const auto config = get_backend_data().CallerConfig;
    shutdown(app);
    init(app, config);
}

bool osc::ui::Context::on_event(Event& ev)
{
    ImGui_ImplOscar_ProcessEvent(ev);

    // handle `.WantCaptureKeyboard`
    constexpr auto keyboard_event_types = std::to_array({EventType::KeyDown, EventType::KeyUp});
    if (ImGui::GetIO().WantCaptureKeyboard and cpp23::contains(keyboard_event_types, ev.type())) {
        return true;
    }

    // handle `.WantCaptureMouse`
    constexpr auto mouse_event_types = std::to_array({EventType::MouseWheel, EventType::MouseMove, EventType::MouseButtonUp, EventType::MouseButtonDown});
    return ImGui::GetIO().WantCaptureMouse and cpp23::contains(mouse_event_types, ev.type());
}

void osc::ui::Context::on_start_new_frame()
{
    App& app = App::upd();

    graphics_backend_on_start_new_frame();
    ImGui_ImplOscar_NewFrame(app);
    ImGui::NewFrame();

    // extra parts
    ImGuizmo::BeginFrame();
}

void osc::ui::Context::render()
{
    {
        OSC_PERF("ImGui::Render()");
        ImGui::Render();
    }

    {
        OSC_PERF("graphics_backend::render(ImGui::GetDrawData())");
        graphics_backend_render(ImGui::GetDrawData());
    }
}

void osc::ui::Context::init(
    App& app,
    CopyOnUpdPtr<ui::ContextConfiguration::Impl> config)
{
    OSC_ASSERT(ImGui::GetCurrentContext() == nullptr && "a global UI context has already been initialized");

    // ensure ImGui uses the same allocator as the rest of
    // our (C++ stdlib) application
    ImGui::SetAllocatorFunctions(
        [](size_t count, [[maybe_unused]] void* user_data) { return ::operator new(count); },
        [](void* ptr, [[maybe_unused]] void* user_data) { ::operator delete(ptr); }
    );

    // init ImGui top-level context
    ImGui::CreateContext();

    // load `imgui.ini`
    load_imgui_config(app.user_data_directory(), app.upd_resource_loader(), *config);

    // setup fonts + styling
    setup_scaling_dependent_rendering_fonts_and_styling(app, *config);

    // init ImGui for oscar
    ImGui_ImplOscar_Init(config, app.main_window_id());

    // init ImGui for oscar's graphics backend (OpenGL)
    graphics_backend_init();

    // init extra parts (plotting, gizmos, etc.)
    ImPlot::CreateContext();
    ImGuizmo::CreateContext();
}

void osc::ui::Context::shutdown(App& app)
{
    ImGuizmo::DestroyContext();
    ImPlot::DestroyContext();

    graphics_backend_shutdown();
    ImGui_ImplOscar_Shutdown(app);
    ImGui::DestroyContext();
}

void osc::ui::align_text_to_frame_padding()
{
    ImGui::AlignTextToFramePadding();
}

void osc::ui::draw_text(CStringView sv)
{
    ImGui::TextUnformatted(sv.data(), sv.data() + sv.size());
}

void osc::ui::detail::draw_text_v(CStringView fmt, va_list args)
{
    ImGui::TextV(fmt.c_str(), args);
}

void osc::ui::detail::draw_text_disabled_v(CStringView fmt, va_list args)
{
    ImGui::TextDisabledV(fmt.c_str(), args);
}

void osc::ui::draw_text_disabled(CStringView sv)
{
    ImGui::TextDisabled("%s", sv.c_str());
}

void osc::ui::draw_text_wrapped(CStringView sv)
{
    ImGui::TextWrapped("%s", sv.c_str());
}

void osc::ui::detail::draw_text_wrapped_v(CStringView fmt, va_list args)
{
    ImGui::TextWrappedV(fmt.c_str(), args);
}

void osc::ui::draw_text_bullet_pointed(CStringView str)
{
    ImGui::BulletText("%s", str.c_str());
}

bool osc::ui::draw_text_link(CStringView str)
{
    return ImGui::TextLink(str.c_str());
}

void osc::ui::draw_bullet_point()
{
    ImGui::Bullet();
}

bool osc::ui::draw_tree_node_ex(CStringView label, ui::TreeNodeFlags flags)
{
    return ImGui::TreeNodeEx(label.c_str(), to<ImGuiTreeNodeFlags>(flags));
}

float osc::ui::get_tree_node_to_label_spacing()
{
    return ImGui::GetTreeNodeToLabelSpacing();
}

void osc::ui::tree_pop()
{
    ImGui::TreePop();
}

void osc::ui::draw_progress_bar(float fraction)
{
    ImGui::ProgressBar(fraction);
}

bool osc::ui::begin_menu(CStringView sv, bool enabled)
{
    return ImGui::BeginMenu(sv.c_str(), enabled);
}

void osc::ui::end_menu()
{
    ImGui::EndMenu();
}

bool osc::ui::draw_menu_item(
    CStringView label,
    std::optional<KeyCombination> shortcut,
    bool selected,
    bool enabled)
{
    return ImGui::MenuItem(label.c_str(), shortcut ? to_human_readable_representation(*shortcut).c_str() : nullptr, selected, enabled);
}

bool osc::ui::draw_menu_item(
    CStringView label,
    std::optional<KeyCombination> shortcut,
    bool* p_selected,
    bool enabled)
{
    return ImGui::MenuItem(label.c_str(), shortcut ? to_human_readable_representation(*shortcut).c_str() : nullptr, p_selected, enabled);
}

bool osc::ui::begin_tab_bar(CStringView str_id)
{
    return ImGui::BeginTabBar(str_id.c_str());
}

void osc::ui::end_tab_bar()
{
    ImGui::EndTabBar();
}

bool osc::ui::begin_tab_item(CStringView label, bool* p_open, TabItemFlags flags)
{
    return ImGui::BeginTabItem(label.c_str(), p_open, to<ImGuiTabItemFlags>(flags));
}

void osc::ui::end_tab_item()
{
    ImGui::EndTabItem();
}

bool osc::ui::draw_tab_item_button(CStringView label)
{
    return ImGui::TabItemButton(label.c_str());
}

void osc::ui::set_num_columns(int count, std::optional<CStringView> id, bool border)
{
    ImGui::Columns(count, id ? id->c_str() : nullptr, border);
}

float osc::ui::get_column_width(int column_index)
{
    return ImGui::GetColumnWidth(column_index);
}

void osc::ui::next_column()
{
    ImGui::NextColumn();
}

void osc::ui::same_line(float offset_from_start_x, float spacing)
{
    ImGui::SameLine(offset_from_start_x, spacing);
}

bool osc::ui::is_mouse_clicked(MouseButton button, bool repeat)
{
    return ImGui::IsMouseClicked(to<ImGuiMouseButton>(button), repeat);
}

bool osc::ui::is_mouse_clicked(MouseButton button, ID owner_id)
{
    return ImGui::IsMouseClicked(to<ImGuiMouseButton>(button), ImGuiInputFlags_None,  owner_id.value());
}

bool osc::ui::is_mouse_released(MouseButton button)
{
    return ImGui::IsMouseReleased(to<ImGuiMouseButton>(button));
}

bool osc::ui::is_mouse_down(MouseButton button)
{
    return ImGui::IsMouseDown(to<ImGuiMouseButton>(button));
}

bool osc::ui::is_mouse_dragging(MouseButton button, float lock_threshold)
{
    return ImGui::IsMouseDragging(to<ImGuiMouseButton>(button), lock_threshold);
}

bool osc::ui::draw_selectable(CStringView label, bool* p_selected)
{
    return ImGui::Selectable(label.c_str(), p_selected);
}

bool osc::ui::draw_selectable(CStringView label, bool selected)
{
    return ImGui::Selectable(label.c_str(), selected);
}

bool osc::ui::draw_checkbox(CStringView label, bool* v)
{
    return ImGui::Checkbox(label.c_str(), v);
}

bool osc::ui::draw_float_slider(CStringView label, float* v, float v_min, float v_max, const char* format, SliderFlags flags)
{
    return ImGui::SliderFloat(label.c_str(), v, v_min, v_max, format, to<ImGuiSliderFlags>(flags));
}

bool osc::ui::draw_scalar_input(CStringView label, DataType data_type, void* p_data, const void* p_step, const void* p_step_fast, const char* format, TextInputFlags flags)
{
    return ImGui::InputScalar(label.c_str(), to<ImGuiDataType>(data_type), p_data, p_step, p_step_fast, format, to<ImGuiInputTextFlags>(flags));
}

bool osc::ui::draw_int_input(CStringView label, int* v, int step, int step_fast, TextInputFlags flags)
{
    return ImGui::InputInt(label.c_str(), v, step, step_fast, to<ImGuiInputTextFlags>(flags));
}

bool osc::ui::draw_size_t_input(CStringView label, size_t* v, size_t step, size_t step_fast, TextInputFlags flags)
{
    return ImGui::InputScalar(label.c_str(), data_type_for<size_t>(), v, &step, &step_fast, nullptr, to<ImGuiInputTextFlags>(flags));
}

bool osc::ui::draw_double_input(CStringView label, double* v, double step, double step_fast, const char* format, TextInputFlags flags)
{
    return ImGui::InputDouble(label.c_str(), v, step, step_fast, format, to<ImGuiInputTextFlags>(flags));
}

bool osc::ui::draw_float_input(CStringView label, float* v, float step, float step_fast, const char* format, TextInputFlags flags)
{
    return ImGui::InputFloat(label.c_str(), v, step, step_fast, format, to<ImGuiInputTextFlags>(flags));
}

bool osc::ui::draw_float3_input(CStringView label, float* v, const char* format, TextInputFlags flags)
{
    return ImGui::InputFloat3(label.c_str(), v, format, to<ImGuiInputTextFlags>(flags));
}

bool osc::ui::draw_vec3_input(CStringView label, Vec3& v, const char* format, TextInputFlags flags)
{
    return ImGui::InputFloat3(label.c_str(), &v.x, format, to<ImGuiInputTextFlags>(flags));
}

bool osc::ui::draw_rgb_color_editor(CStringView label, Color& color)
{
    return ImGui::ColorEdit3(label.c_str(), value_ptr(color));
}

bool osc::ui::draw_rgba_color_editor(CStringView label, Color& color)
{
    return ImGui::ColorEdit4(label.c_str(), value_ptr(color));
}

bool osc::ui::draw_button(CStringView label, const Vec2& size)
{
    return ImGui::Button(label.c_str(), size);
}

bool osc::ui::draw_small_button(CStringView label)
{
    return ImGui::SmallButton(label.c_str());
}

bool osc::ui::draw_arrow_down_button(CStringView label)
{
    return ImGui::ArrowButton(label.c_str(), ImGuiDir_Down);
}

bool osc::ui::draw_invisible_button(CStringView label, Vec2 size)
{
    return ImGui::InvisibleButton(label.c_str(), size);
}

bool osc::ui::draw_radio_button(CStringView label, bool active)
{
    return ImGui::RadioButton(label.c_str(), active);
}

bool osc::ui::draw_collapsing_header(CStringView label, TreeNodeFlags flags)
{
    return ImGui::CollapsingHeader(label.c_str(), to<ImGuiTreeNodeFlags>(flags));
}

void osc::ui::draw_dummy(const Vec2& size)
{
    ImGui::Dummy(size);
}

void osc::ui::draw_vertical_spacer(float num_lines)
{
    ImGui::Dummy({0.0f, num_lines * get_text_line_height_in_current_panel()});
}

bool osc::ui::begin_combobox(CStringView label, CStringView preview_value, ComboFlags flags)
{
    return ImGui::BeginCombo(label.c_str(), preview_value.empty() ? nullptr : preview_value.c_str(), to<ImGuiComboFlags>(flags));
}

void osc::ui::end_combobox()
{
    ImGui::EndCombo();
}

bool osc::ui::begin_listbox(CStringView label)
{
    return ImGui::BeginListBox(label.c_str());
}

void osc::ui::end_listbox()
{
    ImGui::EndListBox();
}

void osc::ui::enable_dockspace_over_main_window()
{
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
}

bool osc::ui::begin_panel(CStringView name, bool* p_open, PanelFlags flags)
{
    return ImGui::Begin(name.c_str(), p_open, to<ImGuiWindowFlags>(flags));
}

void osc::ui::end_panel()
{
    ImGui::End();
}

bool osc::ui::begin_child_panel(CStringView str_id, const Vec2& size, ChildPanelFlags child_flags, PanelFlags panel_flags)
{
    return ImGui::BeginChild(str_id.c_str(), size, to<ImGuiChildFlags>(child_flags), to<ImGuiWindowFlags>(panel_flags));
}

void osc::ui::end_child_panel()
{
    ImGui::EndChild();
}

void osc::ui::close_current_popup()
{
    ImGui::CloseCurrentPopup();
}

void osc::ui::detail::set_tooltip_v(CStringView fmt, va_list args)
{
    ImGui::SetTooltipV(fmt.c_str(), args);
}

void osc::ui::set_scroll_y_here()
{
    ImGui::SetScrollHereY();
}

float osc::ui::get_frame_height()
{
    return ImGui::GetFrameHeight();
}

Vec2 osc::ui::get_content_region_available()
{
    return ImGui::GetContentRegionAvail();
}

Vec2 osc::ui::get_cursor_start_panel_pos()
{
    return ImGui::GetCursorStartPos();
}

Vec2 osc::ui::get_cursor_panel_pos()
{
    return ImGui::GetCursorPos();
}

void osc::ui::set_cursor_panel_pos(Vec2 pos)
{
    ImGui::SetCursorPos(pos);
}

float osc::ui::get_cursor_panel_pos_x()
{
    return ImGui::GetCursorPosX();
}

void osc::ui::set_cursor_panel_pos_x(float local_x)
{
    ImGui::SetCursorPosX(local_x);
}

Vec2 osc::ui::get_cursor_ui_pos()
{
    return ImGui::GetCursorScreenPos();
}

void osc::ui::set_cursor_ui_pos(Vec2 pos)
{
    ImGui::SetCursorScreenPos(pos);
}

void osc::ui::set_next_panel_ui_pos(Vec2 pos, Conditional conditional, Vec2 pivot)
{
    ImGui::SetNextWindowPos(pos, to<ImGuiCond>(conditional), pivot);
}

void osc::ui::set_next_panel_size(Vec2 size, Conditional conditional)
{
    ImGui::SetNextWindowSize(size, to<ImGuiCond>(conditional));
}

void osc::ui::set_next_panel_size_constraints(Vec2 size_min, Vec2 size_max)
{
    ImGui::SetNextWindowSizeConstraints(size_min, size_max);
}

void osc::ui::set_next_panel_bg_alpha(float alpha)
{
    ImGui::SetNextWindowBgAlpha(alpha);
}

bool osc::ui::is_panel_hovered(HoveredFlags flags)
{
    return ImGui::IsWindowHovered(to<ImGuiHoveredFlags>(flags));
}

void osc::ui::begin_disabled(bool disabled)
{
    ImGui::BeginDisabled(disabled);
}

void osc::ui::end_disabled()
{
    ImGui::EndDisabled();
}

bool osc::ui::begin_tooltip_nowrap()
{
    return ImGui::BeginTooltip();
}

void osc::ui::end_tooltip_nowrap()
{
    ImGui::EndTooltip();
}

void osc::ui::push_id(UID id)
{
    ImGui::PushID(static_cast<int>(id.get()));
}

void osc::ui::push_id(int id)
{
    ImGui::PushID(id);
}

void osc::ui::push_id(const void* id)
{
    ImGui::PushID(id);
}

void osc::ui::push_id(std::string_view str_id)
{
    ImGui::PushID(str_id.data(), str_id.data() + str_id.size());
}

void osc::ui::pop_id()
{
    ImGui::PopID();
}

ui::ID osc::ui::get_id(std::string_view str_id)
{
    return ID{ImGui::GetID(str_id.data(), str_id.data() + str_id.size())};
}

void osc::ui::set_next_item_size(Rect r)  // note: ImGui API assumes cursor is located at `p1` already
{
    ImGui::ItemSize(ImRect{r.p1, r.p2});
}

bool osc::ui::add_item(Rect bounds, ID id)
{
    return ImGui::ItemAdd(ImRect{bounds.p1, bounds.p2}, id.value());
}

bool osc::ui::is_item_hoverable(Rect bounds, ID id)
{
    return ImGui::ItemHoverable(ImRect{bounds.p1, bounds.p2}, id.value(), ImGui::GetItemFlags());
}

void osc::ui::draw_separator()
{
    ImGui::Separator();
}

void osc::ui::start_new_line()
{
    ImGui::NewLine();
}

void osc::ui::indent(float indent_w)
{
    ImGui::Indent(indent_w);
}

void osc::ui::unindent(float indent_w)
{
    ImGui::Unindent(indent_w);
}

void osc::ui::set_keyboard_focus_here()
{
    ImGui::SetKeyboardFocusHere();
}

bool osc::ui::is_key_pressed(Key key, bool repeat)
{
    return ImGui::IsKeyPressed(to<ImGuiKey>(key), repeat);
}

bool osc::ui::is_key_released(Key key)
{
    return ImGui::IsKeyReleased(to<ImGuiKey>(key));
}

bool osc::ui::is_key_down(Key key)
{
    return ImGui::IsKeyDown(to<ImGuiKey>(key));
}

Color osc::ui::get_style_color(ColorVar color)
{
    return Color{ImGui::GetStyleColorVec4(to<ImGuiCol>(color))};
}

Vec2 osc::ui::get_style_frame_padding()
{
    return ImGui::GetStyle().FramePadding;
}

float osc::ui::get_style_frame_border_size()
{
    return ImGui::GetStyle().FrameBorderSize;
}

Vec2 osc::ui::get_style_panel_padding()
{
    return ImGui::GetStyle().WindowPadding;
}

Vec2 osc::ui::get_style_item_spacing()
{
    return ImGui::GetStyle().ItemSpacing;
}

Vec2 osc::ui::get_style_item_inner_spacing()
{
    return ImGui::GetStyle().ItemInnerSpacing;
}

float osc::ui::get_style_alpha()
{
    return ImGui::GetStyle().Alpha;
}

float osc::ui::get_framerate()
{
    return ImGui::GetIO().Framerate;
}

bool osc::ui::wants_keyboard()
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

void osc::ui::push_style_var(StyleVar var, Vec2 pos)
{
    ImGui::PushStyleVar(to<ImGuiStyleVar>(var), pos);
}

void osc::ui::push_style_var(StyleVar var, float pos)
{
    ImGui::PushStyleVar(to<ImGuiStyleVar>(var), pos);
}

void osc::ui::pop_style_var(int count)
{
    ImGui::PopStyleVar(count);
}

void osc::ui::open_popup(CStringView str_id, PopupFlags popup_flags)
{
    ImGui::OpenPopup(str_id.c_str(), to<ImGuiPopupFlags>(popup_flags));
}

bool osc::ui::begin_popup(CStringView str_id, PanelFlags flags)
{
    return ImGui::BeginPopup(str_id.c_str(), to<ImGuiWindowFlags>(flags));
}

bool osc::ui::begin_popup_context_menu(CStringView str_id, PopupFlags popup_flags)
{
    return ImGui::BeginPopupContextItem(str_id.c_str(), to<ImGuiPopupFlags>(popup_flags));
}

bool osc::ui::begin_popup_modal(CStringView name, bool* p_open, PanelFlags flags)
{
    return ImGui::BeginPopupModal(name.c_str(), p_open, to<ImGuiWindowFlags>(flags));
}

void osc::ui::end_popup()
{
    ImGui::EndPopup();
}

Vec2 osc::ui::get_mouse_ui_pos()
{
    return ImGui::GetMousePos();
}

bool osc::ui::begin_menu_bar()
{
    return ImGui::BeginMenuBar();
}

void osc::ui::end_menu_bar()
{
    ImGui::EndMenuBar();
}

void osc::ui::hide_mouse_cursor()
{
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
}

void osc::ui::show_mouse_cursor()
{
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
}

void osc::ui::set_next_item_width(float item_width)
{
    ImGui::SetNextItemWidth(item_width);
}

void osc::ui::set_next_item_open(bool is_open)
{
    ImGui::SetNextItemOpen(is_open);
}

void osc::ui::push_item_flag(ItemFlags flags, bool enabled)
{
    ImGui::PushItemFlag(to<ImGuiItemFlags>(flags), enabled);
}

void osc::ui::pop_item_flag()
{
    ImGui::PopItemFlag();
}

bool osc::ui::is_item_clicked(MouseButton mouse_button)
{
    return ImGui::IsItemClicked(to<ImGuiMouseButton>(mouse_button));
}

bool osc::ui::is_item_hovered(HoveredFlags flags)
{
    return ImGui::IsItemHovered(to<ImGuiHoveredFlags>(flags));
}

bool osc::ui::is_item_deactivated_after_edit()
{
    return ImGui::IsItemDeactivatedAfterEdit();
}

Vec2 osc::ui::get_item_top_left_ui_pos()
{
    return ImGui::GetItemRectMin();
}

Vec2 osc::ui::get_item_bottom_right_ui_pos()
{
    return ImGui::GetItemRectMax();
}

bool osc::ui::begin_table(CStringView str_id, int column, TableFlags flags, const Vec2& outer_size, float inner_width)
{
    return ImGui::BeginTable(str_id.c_str(), column, to<ImGuiTableFlags>(flags), outer_size, inner_width);
}

void osc::ui::table_setup_scroll_freeze(int cols, int rows)
{
    ImGui::TableSetupScrollFreeze(cols, rows);
}

bool osc::ui::table_column_sort_specs_are_dirty()
{
    const ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs();
    return (specs != nullptr) and specs->SpecsDirty;
}

std::vector<ui::TableColumnSortSpec> osc::ui::get_table_column_sort_specs()
{
    std::vector<TableColumnSortSpec> rv;
    const ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs();
    if (specs) {
        rv.reserve(specs->SpecsCount);
        for (int i = 0; i < specs->SpecsCount; ++i) {
            rv.push_back(to<TableColumnSortSpec>(specs->Specs[i]));
        }
    }
    return rv;
}

void osc::ui::table_headers_row()
{
    ImGui::TableHeadersRow();
}

bool osc::ui::table_set_column_index(int column_n)
{
    return ImGui::TableSetColumnIndex(column_n);
}

void osc::ui::table_next_row()
{
    ImGui::TableNextRow(0, 0.0f);
}

void osc::ui::table_setup_column(CStringView label, ColumnFlags flags, float init_width_or_weight, ID user_id)
{
    ImGui::TableSetupColumn(label.c_str(), to<ImGuiTableColumnFlags>(flags), init_width_or_weight, user_id.value());
}

void osc::ui::end_table()
{
    ImGui::EndTable();
}

void osc::ui::push_style_color(ColorVar var, const Color& c)
{
    ImGui::PushStyleColor(to<ImGuiCol>(var), ImVec4{c});
}

void osc::ui::pop_style_color(int count)
{
    ImGui::PopStyleColor(count);
}

Color osc::ui::get_color(ColorVar var)
{
    return ImGui::GetStyle().Colors[to<ImGuiCol>(var)];
}

float osc::ui::get_text_line_height_in_current_panel()
{
    OSC_ASSERT_ALWAYS(ImGui::GetCurrentWindow() && "not currently in a panel (use ui::get_font_base_size if you want a panel-independent size)");
    return ImGui::GetTextLineHeight();
}

float osc::ui::get_text_line_height_with_spacing_in_current_panel()
{
    OSC_ASSERT_ALWAYS(ImGui::GetCurrentWindow() && "not currently in a panel (use ui::get_font_base_size if you want a panel-independent size)");
    return ImGui::GetTextLineHeightWithSpacing();
}

float osc::ui::get_font_base_size()
{
    // HACK: context should be set up to return this, but font initialization is lazy in imgui
    return c_default_base_font_pixel_size;
}

float osc::ui::get_font_base_size_with_spacing()
{
    // HACK: context should be set up to return this, but font initialization is lazy in imgui
    return c_default_base_font_pixel_size + ImGui::GetStyle().ItemSpacing.y;
}

Vec2 osc::ui::calc_text_size(CStringView text, bool hide_text_after_double_hash)
{
    return ImGui::CalcTextSize(text.c_str(), text.c_str() + text.size(), hide_text_after_double_hash);
}

Vec2 osc::ui::get_panel_size()
{
    return ImGui::GetWindowSize();
}

void osc::ui::DrawListAPI::add_rect(const Rect& ui_rect, const Color& color, float rounding, float thickness)
{
    impl_get_drawlist().AddRect(ui_rect.p1, ui_rect.p2, to_ImU32(color), rounding, 0, thickness);
}

void osc::ui::DrawListAPI::add_rect_filled(const Rect& ui_rect, const Color& color, float rounding)
{
    impl_get_drawlist().AddRectFilled(ui_rect.p1, ui_rect.p2, to_ImU32(color), rounding);
}

void osc::ui::DrawListAPI::add_circle(const Circle& ui_circle, const Color& color, int num_segments, float thickness)
{
    impl_get_drawlist().AddCircle(ui_circle.origin, ui_circle.radius, to_ImU32(color), num_segments, thickness);
}

void osc::ui::DrawListAPI::add_circle_filled(const Circle& ui_circle, const Color& color, int num_segments)
{
    impl_get_drawlist().AddCircleFilled(ui_circle.origin, ui_circle.radius, to_ImU32(color), num_segments);
}

void osc::ui::DrawListAPI::add_text(const Vec2& ui_position, const Color& color, CStringView text)
{
    impl_get_drawlist().AddText(ui_position, to_ImU32(color), text.c_str(), text.c_str() + text.size());
}

void osc::ui::DrawListAPI::add_line(const Vec2& ui_start, const Vec2& ui_end, const Color& color, float thickness)
{
    impl_get_drawlist().AddLine(ui_start, ui_end, to_ImU32(color), thickness);
}

void osc::ui::DrawListAPI::add_triangle_filled(const Vec2 ui_p0, const Vec2& ui_p1, const Vec2& ui_p2, const Color& color)
{
    impl_get_drawlist().AddTriangleFilled(ui_p0, ui_p1, ui_p2, to_ImU32(color));
}

void osc::ui::DrawListAPI::push_clip_rect(const Rect& r, bool intersect_with_currect_clip_rect)
{
    impl_get_drawlist().PushClipRect(r.p1, r.p2, intersect_with_currect_clip_rect);
}

void osc::ui::DrawListAPI::pop_clip_rect()
{
    impl_get_drawlist().PopClipRect();
}

void osc::ui::DrawListAPI::render_to(RenderTexture& target)
{
    ImDrawList& drawlist = impl_get_drawlist();

    ImDrawData data;
    data.Valid = true;
    data.CmdListsCount = 1;
    data.TotalIdxCount = drawlist.VtxBuffer.Size;
    data.TotalIdxCount = drawlist.IdxBuffer.Size;
    data.CmdLists.push_back(&drawlist);
    data.DisplayPos = {0.0f, 0.0f};
    data.DisplaySize = ImVec2{target.dimensions()};
    data.FramebufferScale = ImGui::GetIO().DisplayFramebufferScale;
    data.OwnerViewport = ImGui::GetMainViewport();

    graphics_backend_render(&data, &target);
}

ui::DrawListView osc::ui::get_panel_draw_list()
{
    return ui::DrawListView{ImGui::GetWindowDrawList()};
}

ui::DrawListView osc::ui::get_foreground_draw_list()
{
    return ui::DrawListView{ImGui::GetForegroundDrawList()};
}

osc::ui::DrawList::DrawList() :
    underlying_drawlist_{std::make_unique<ImDrawList>(ImGui::GetDrawListSharedData())}
{
    underlying_drawlist_->Flags |= ImDrawListFlags_AntiAliasedLines;
    underlying_drawlist_->AddDrawCmd();
}
osc::ui::DrawList::DrawList(DrawList&&) noexcept = default;
osc::ui::DrawList& osc::ui::DrawList::operator=(DrawList&&) noexcept = default;
osc::ui::DrawList::~DrawList() noexcept = default;


void osc::ui::show_demo_panel()
{
    ImGui::ShowDemoWindow();
}

void osc::ui::apply_dark_theme()
{
    // see: https://github.com/ocornut/imgui/issues/707
    // this one: https://github.com/ocornut/imgui/issues/707#issuecomment-512669512

    auto& style = ImGui::GetStyle();

    style.FrameRounding = 0.0f;
    style.GrabRounding = 20.0f;
    style.GrabMinSize = 10.0f;

    auto& colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.2f, 0.22f, 0.24f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.24f, 0.32f, 0.35f, 0.70f);  // contrasts against other Header* elements (#677)
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

    // Make modal windows pop up immediately without a fade-in effect, because
    // oscar UIs might be running in an event-driven mode.
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4{0.0f, 0.0f, 0.0f, 0.0f};
}

bool osc::ui::update_polar_camera_from_mouse_inputs(
    PolarPerspectiveCamera& camera,
    Vec2 viewport_dimensions)
{
    bool modified = false;

    // handle mousewheel scrolling
    if (const float wheel = ImGui::GetIO().MouseWheel; wheel != 0.0f) {
        // careful: different operating systems have different orders of
        // of magnitude and frequency for scroll events, so this section
        // needs to make sure that the user can't (e.g.) zoom in too much
        // or too quickly (MacOS used to have aggressive scrolling, #971).
        float r = camera.radius * (1.0f - 0.1f*wheel);
        r = clamp(r, 0.2f*camera.radius, 5.0f*camera.radius);  // clamp how much zooming can happen in one go
        r = clamp(r, 0.0001f, 1000.0f);  // clamp absolute amount between 0.1 mm and 1 km
        camera.radius = r;
        modified = true;
    }

    // these camera controls try to be the union of other GUIs (e.g. Blender)
    //
    // left drag: drags/orbits camera
    // left drag + L/R SHIFT: pans camera (CUSTOM behavior: can be handy on laptops where right-click + drag sucks)
    // left drag + L/R CTRL: zoom camera (CUSTOM behavior: can be handy on laptops where right-click + drag sucks)
    // middle drag: drags/orbits camera (Blender behavior)
    // middle drag + L/R SHIFT: pans camera (Blender behavior)
    // middle drag + L/R CTRL: zooms camera (Blender behavior)
    // right drag: pans camera
    //
    // the reason it's like this is to please legacy users from a variety of
    // other GUIs and users who use modelling software like Blender (which is
    // more popular among newer users looking to make new models)

    const float aspect_ratio = aspect_ratio_of(viewport_dimensions);

    const bool left_dragging = ui::is_mouse_dragging(MouseButton::Left);
    const bool middle_dragging = ui::is_mouse_dragging(MouseButton::Middle);
    const Vec2 delta = ImGui::GetIO().MouseDelta;

    if (delta != Vec2{} and (left_dragging or middle_dragging)) {
        if (is_ctrl_down()) {
            camera.pan(aspect_ratio, delta/viewport_dimensions);
            modified = true;
        }
        else if (is_ctrl_or_super_down()) {
            camera.radius *= 1.0f + 4.0f*delta.y/viewport_dimensions.y;
            modified = true;
        }
        else {
            camera.drag(delta/viewport_dimensions);
            modified = true;
        }
    }
    else if (ui::is_mouse_dragging(MouseButton::Right)) {
        if (is_alt_down()) {
            camera.radius *= 1.0f + 4.0f*delta.y/viewport_dimensions.y;
            modified = true;
        }
        else {
            camera.pan(aspect_ratio, delta/viewport_dimensions);
            modified = true;
        }
    }

    if (modified) {
        camera.rescale_znear_and_zfar_based_on_radius();
    }

    return modified;
}

bool osc::ui::update_polar_camera_from_keyboard_inputs(
    PolarPerspectiveCamera& camera,
    const Rect& viewport_rect,
    std::optional<AABB> maybe_scene_world_space_aabb)
{
    const bool shift_down = is_shift_down();
    const bool ctrl_or_super_down = is_ctrl_or_super_down();

    if (ui::is_key_released(Key::X)) {
        if (ctrl_or_super_down) {
            focus_along_minus_x(camera);
            return true;
        }
        else {
            focus_along_x(camera);
            return true;
        }
    }
    else if (ui::is_key_pressed(Key::Y)) {
        // Ctrl+Y already does something?
        if (not ctrl_or_super_down) {
            focus_along_y(camera);
            return true;
        }
    }
    else if (ui::is_key_pressed(Key::F)) {
        if (ctrl_or_super_down) {
            if (maybe_scene_world_space_aabb) {
                auto_focus(
                    camera,
                    *maybe_scene_world_space_aabb,
                    aspect_ratio_of(viewport_rect)
                );
                return true;
            }
        }
        else {
            reset(camera);
            return true;
        }
    }
    else if (ctrl_or_super_down and ui::is_key_pressed(Key::_8)) {
        if (maybe_scene_world_space_aabb) {
            auto_focus(
                camera,
                *maybe_scene_world_space_aabb,
                aspect_ratio_of(viewport_rect)
            );
            return true;
        }
    }
    else if (ui::is_key_down(Key::UpArrow)) {
        if (ctrl_or_super_down) {
            // pan
            camera.pan(aspect_ratio_of(viewport_rect), {0.0f, -0.1f});
        }
        else if (shift_down) {
            camera.phi -= 90_deg;  // rotate in 90-deg increments
        }
        else {
            camera.phi -= 10_deg;  // rotate in 10-deg increments
        }
        return true;
    }
    else if (ui::is_key_down(Key::DownArrow)) {
        if (ctrl_or_super_down) {
            // pan
            camera.pan(aspect_ratio_of(viewport_rect), {0.0f, +0.1f});
        }
        else if (shift_down) {
            // rotate in 90-deg increments
            camera.phi += 90_deg;
        }
        else {
            // rotate in 10-deg increments
            camera.phi += 10_deg;
        }
        return true;
    }
    else if (ui::is_key_down(Key::LeftArrow)) {
        if (ctrl_or_super_down) {
            // pan
            camera.pan(aspect_ratio_of(viewport_rect), {-0.1f, 0.0f});
        }
        else if (shift_down) {
            // rotate in 90-deg increments
            camera.theta += 90_deg;
        }
        else {
            // rotate in 10-deg increments
            camera.theta += 10_deg;
        }
        return true;
    }
    else if (ui::is_key_down(Key::RightArrow)) {
        if (ctrl_or_super_down) {
            // pan
            camera.pan(aspect_ratio_of(viewport_rect), {+0.1f, 0.0f});
        }
        else if (shift_down) {
            camera.theta -= 90_deg;  // rotate in 90-deg increments
        }
        else {
            camera.theta -= 10_deg;  // rotate in 10-deg increments
        }
        return true;
    }
    else if (ui::is_key_down(Key::Minus)) {
        camera.radius *= 1.1f;
        return true;
    }
    else if (ui::is_key_down(Key::Equals)) {
        camera.radius *= 0.9f;
        return true;
    }
    return false;
}

bool osc::ui::update_polar_camera_from_all_inputs(
    PolarPerspectiveCamera& camera,
    const Rect& viewport_rect,
    std::optional<AABB> maybe_scene_world_space_aabb)
{
    const ImGuiIO& io = ImGui::GetIO();

    // we don't check `io.WantCaptureMouse` because clicking/dragging on an `ImGui::Image`
    // is classed as a mouse interaction
    const bool mouse_handled =
        update_polar_camera_from_mouse_inputs(camera, dimensions_of(viewport_rect));

    const bool keyboard_handled = not io.WantCaptureKeyboard ?
        update_polar_camera_from_keyboard_inputs(camera, viewport_rect, maybe_scene_world_space_aabb) :
        false;

    return mouse_handled or keyboard_handled;
}

void osc::ui::update_camera_from_all_inputs(Camera& camera, EulerAngles& eulers)
{
    const Vec3 front = camera.direction();
    const Vec3 up = camera.upwards_direction();
    const Vec3 right = cross(front, up);
    const Vec2 mouseDelta = ImGui::GetIO().MouseDelta;

    const float speed = 10.0f;
    const float displacement = speed * ImGui::GetIO().DeltaTime;
    const auto sensitivity = 0.005_rad;

    // keyboard: changes camera position
    Vec3 pos = camera.position();
    if (ui::is_key_down(Key::W)) {
        pos += displacement * front;
    }
    if (ui::is_key_down(Key::S)) {
        pos -= displacement * front;
    }
    if (ui::is_key_down(Key::A)) {
        pos -= displacement * right;
    }
    if (ui::is_key_down(Key::D)) {
        pos += displacement * right;
    }
    if (ui::is_key_down(Key::Space)) {
        pos += displacement * up;
    }
    if (ui::is_ctrl_down()) {
        pos -= displacement * up;
    }
    camera.set_position(pos);

    eulers.x += sensitivity * -mouseDelta.y;
    eulers.x = clamp(eulers.x, -90_deg + 0.1_rad, 90_deg - 0.1_rad);
    eulers.y += sensitivity * -mouseDelta.x;
    eulers.y = mod(eulers.y, 360_deg);

    camera.set_rotation(to_world_space_rotation_quat(eulers));
}

Rect osc::ui::content_region_available_ui_rect()
{
    const Vec2 top_left = ui::get_cursor_ui_pos();
    return Rect{top_left, top_left + ui::get_content_region_available()};
}

void osc::ui::draw_image(
    const Texture2D& texture,
    std::optional<Vec2> dimensions,
    const Rect& region_uv_coordinates)
{
    if (not dimensions) {
        dimensions = texture.dimensions();
    }
    const Vec2 top_left = {region_uv_coordinates.p1.x, 1.0f - region_uv_coordinates.p1.y};
    const Vec2 bottom_right = {region_uv_coordinates.p2.x, 1.0f - region_uv_coordinates.p2.y};
    const auto handle = graphics_backend_allocate_texture_for_current_frame(texture);
    ImGui::Image(handle, *dimensions, top_left, bottom_right);
}

void osc::ui::draw_image(const RenderTexture& texture)
{
    draw_image(texture, texture.device_independent_dimensions());
}

void osc::ui::draw_image(const RenderTexture& texture, Vec2 dimensions)
{
    const Vec2 uv0 = {0.0f, 1.0f};
    const Vec2 uv1 = {1.0f, 0.0f};
    const auto handle = graphics_backend_allocate_texture_for_current_frame(texture);
    ImGui::Image(handle, dimensions, uv0, uv1);
}

Vec2 osc::ui::calc_button_size(CStringView content)
{
    return ui::calc_text_size(content) + 2.0f*ui::get_style_frame_padding();
}

float osc::ui::calc_button_width(CStringView content)
{
    return calc_button_size(content).x;
}

bool osc::ui::draw_button_nobg(CStringView label, Vec2 dimensions)
{
    push_style_color(ColorVar::Button, Color::clear());
    push_style_color(ColorVar::ButtonHovered, Color::clear());
    const bool rv = ui::draw_button(label, dimensions);
    pop_style_color(2);

    return rv;
}

bool osc::ui::draw_image_button(
    CStringView label,
    const Texture2D& texture,
    Vec2 dimensions,
    const Rect& texture_coordinates)
{
    const auto handle = graphics_backend_allocate_texture_for_current_frame(texture);
    return ImGui::ImageButton(label.c_str(), handle, dimensions, texture_coordinates.p1, texture_coordinates.p2);
}

bool osc::ui::draw_image_button(CStringView label, const Texture2D& texture, Vec2 dimensions)
{
    return draw_image_button(label, texture, dimensions, Rect{{0.0f, 1.0f}, {1.0f, 0.0f}});
}

Rect osc::ui::get_last_drawn_item_ui_rect()
{
    return {ui::get_item_top_left_ui_pos(), ui::get_item_bottom_right_ui_pos()};
}

Rect osc::ui::get_last_drawn_item_screen_rect()
{
    const Rect ui_rect = get_last_drawn_item_ui_rect();
    const Vec2 r = ImGui::GetIO().DisplaySize;
    return Rect{
        {ui_rect.p1.x, r.y - ui_rect.p2.y},
        {ui_rect.p2.x, r.y - ui_rect.p1.y},
    };
}

void osc::ui::add_screenshot_annotation_to_last_drawn_item(std::string_view label)
{
    App::upd().add_main_window_frame_annotation(label, get_last_drawn_item_screen_rect());
}

ui::HittestResult osc::ui::hittest_last_drawn_item()
{
    return hittest_last_drawn_item(c_default_drag_threshold);
}

ui::HittestResult osc::ui::hittest_last_drawn_item(float drag_threshold)
{
    HittestResult rv;
    rv.item_ui_rect.p1 = ui::get_item_top_left_ui_pos();
    rv.item_ui_rect.p2 = ui::get_item_bottom_right_ui_pos();
    rv.is_hovered = ui::is_item_hovered();
    rv.is_left_click_released_without_dragging = rv.is_hovered and is_mouse_released_without_dragging(MouseButton::Left, drag_threshold);
    rv.is_right_click_released_without_dragging = rv.is_hovered and is_mouse_released_without_dragging(MouseButton::Right, drag_threshold);
    return rv;
}

bool osc::ui::any_of_keys_down(std::span<const Key> keys)
{
    return rgs::any_of(keys, ui::is_key_down);
}

bool osc::ui::any_of_keys_down(std::initializer_list<const Key> keys)
{
    return any_of_keys_down(std::span<Key const>{keys.begin(), keys.end()});
}

bool osc::ui::any_of_keys_pressed(std::span<const Key> keys)
{
    return rgs::any_of(keys, [](Key k) { return ui::is_key_pressed(k); });
}
bool osc::ui::any_of_keys_pressed(std::initializer_list<const Key> keys)
{
    return any_of_keys_pressed(std::span<const Key>{keys.begin(), keys.end()});
}

bool osc::ui::is_ctrl_down()
{
    return ImGui::GetIO().KeyCtrl;
}

bool osc::ui::is_ctrl_or_super_down()
{
    return ImGui::GetIO().KeyCtrl or ImGui::GetIO().KeySuper;
}

bool osc::ui::is_shift_down()
{
    return ImGui::GetIO().KeyShift;
}

bool osc::ui::is_alt_down()
{
    return ImGui::GetIO().KeyAlt;
}

bool osc::ui::is_mouse_released_without_dragging(MouseButton mouse_button)
{
    return is_mouse_released_without_dragging(mouse_button, c_default_drag_threshold);
}

bool osc::ui::is_mouse_released_without_dragging(MouseButton mouse_button, float threshold)
{
    if (not ui::is_mouse_released(mouse_button)) {
        return false;
    }

    const Vec2 drag_delta = ImGui::GetMouseDragDelta(to<ImGuiMouseButton>(mouse_button));

    return length(drag_delta) < threshold;
}

bool osc::ui::is_mouse_dragging_with_any_button_down()
{
    return
        ui::is_mouse_dragging(MouseButton::Left) or
        ui::is_mouse_dragging(MouseButton::Middle) or
        ui::is_mouse_dragging(MouseButton::Right);
}

void osc::ui::begin_tooltip(std::optional<float> wrap_width)
{
    begin_tooltip_nowrap();
    ImGui::PushTextWrapPos(wrap_width.value_or(ImGui::GetFontSize() * 35.0f));
}

void osc::ui::end_tooltip(std::optional<float>)
{
    ImGui::PopTextWrapPos();
    end_tooltip_nowrap();
}

void osc::ui::draw_tooltip_header_text(CStringView content)
{
    draw_text(content);
}

void osc::ui::draw_tooltip_description_spacer()
{
    ui::draw_vertical_spacer(1.0f/15.0f);
}

void osc::ui::draw_tooltip_description_text(CStringView content)
{
    draw_text_faded(content);
}

void osc::ui::draw_tooltip_body_only(CStringView content)
{
    begin_tooltip();
    draw_tooltip_header_text(content);
    end_tooltip();
}

void osc::ui::draw_tooltip_body_only_if_item_hovered(
    CStringView content,
    HoveredFlags flags)
{
    if (ui::is_item_hovered(flags)) {
        draw_tooltip_body_only(content);
    }
}

void osc::ui::draw_tooltip(CStringView header, CStringView description)
{
    begin_tooltip();
    draw_tooltip_header_text(header);
    if (not description.empty()) {
        draw_tooltip_description_spacer();
        draw_tooltip_description_text(description);
    }
    end_tooltip();
}

void osc::ui::draw_tooltip_if_item_hovered(
    CStringView header,
    CStringView description,
    HoveredFlags flags)
{
    if (ui::is_item_hovered(flags)) {
        draw_tooltip(header, description);
    }
}

void osc::ui::draw_help_marker(CStringView header, CStringView desc)
{
    ui::draw_text_disabled("(?)");
    draw_tooltip_if_item_hovered(header, desc);
}

void osc::ui::draw_help_marker(CStringView content)
{
    ui::draw_text_disabled("(?)");
    draw_tooltip_if_item_hovered(content, {});
}

bool osc::ui::draw_string_input(CStringView label, std::string& edited_string, TextInputFlags flags)
{
    return ImGui::InputText(label.c_str(), &edited_string, to<ImGuiInputTextFlags>(flags));
}

bool osc::ui::draw_string_input_with_hint(CStringView label, CStringView hint, std::string& edited_string, TextInputFlags flags)
{
    return ImGui::InputTextWithHint(label.c_str(), hint.c_str(), & edited_string, to<ImGuiInputTextFlags>(flags));
}

bool osc::ui::draw_float_meters_input(CStringView label, float& v, float step, float step_fast, TextInputFlags flags)
{
    return ui::draw_float_input(label, &v, step, step_fast, "%.6f", flags);
}

bool osc::ui::draw_float3_meters_input(CStringView label, Vec3& vec, TextInputFlags flags)
{
    return ui::draw_float3_input(label, value_ptr(vec), "%.6f", flags);
}

bool osc::ui::draw_float_meters_slider(CStringView label, float& v, float v_min, float v_max, SliderFlags flags)
{
    return ui::draw_float_slider(label, &v, v_min, v_max, "%.6f", flags);
}

bool osc::ui::draw_float_kilogram_input(CStringView label, float& v, float step, float step_fast, TextInputFlags flags)
{
    return draw_float_meters_input(label, v, step, step_fast, flags);
}

bool osc::ui::draw_angle_input(CStringView label, Radians& v)
{
    float degrees_float = Degrees{v}.count();
    if (ui::draw_float_input(label, &degrees_float)) {
        v = Radians{Degrees{degrees_float}};
        return true;
    }
    return false;
}

bool osc::ui::draw_angle3_input(
    CStringView label,
    Vec<3, Radians>& angles,
    CStringView format)
{
    Vec3 dvs = {Degrees{angles.x}.count(), Degrees{angles.y}.count(), Degrees{angles.z}.count()};
    if (ui::draw_vec3_input(label, dvs, format.c_str())) {
        angles = Vec<3, Radians>{Vec<3, Degrees>{dvs}};
        return true;
    }
    return false;
}

bool osc::ui::draw_angle_slider(CStringView label, Radians& v, Radians min, Radians max)
{
    float degrees_float = Degrees{v}.count();
    const Degrees degrees_min{min};
    const Degrees degrees_max{max};
    if (ui::draw_float_slider(label, &degrees_float, degrees_min.count(), degrees_max.count())) {
        v = Radians{Degrees{degrees_float}};
        return true;
    }
    return false;
}

ui::PanelFlags osc::ui::get_minimal_panel_flags()
{
    return {
        ui::PanelFlag::NoBackground,
        ui::PanelFlag::NoCollapse,
        ui::PanelFlag::NoDecoration,
        ui::PanelFlag::NoDocking,
        ui::PanelFlag::NoInputs,
        ui::PanelFlag::NoMove,
        ui::PanelFlag::NoNav,
        ui::PanelFlag::NoResize,
        ui::PanelFlag::NoSavedSettings,
        ui::PanelFlag::NoScrollbar,
        ui::PanelFlag::NoTitleBar,
    };
}

bool osc::ui::main_window_has_workspace()
{
    return area_of(get_main_window_workspace_ui_rect()) > 0.0f;
}


Rect osc::ui::get_main_window_workspace_ui_rect()
{
    const ImGuiViewport& viewport = *ImGui::GetMainViewport();

    return Rect{
        viewport.WorkPos,
        Vec2{viewport.WorkPos} + Vec2{viewport.WorkSize}
    };
}

Rect osc::ui::get_main_window_workspace_screen_space_rect()
{
    const ImGuiViewport& viewport = *ImGui::GetMainViewport();
    const Vec2 bottom_left_ui_space = Vec2{viewport.WorkPos} + Vec2{0.0f, viewport.WorkSize.y};
    const Vec2 bottom_left_screen_space = Vec2{bottom_left_ui_space.x, viewport.Size.y - bottom_left_ui_space.y};
    const Vec2 top_right_screen_space = bottom_left_screen_space + Vec2{viewport.WorkSize};

    return {bottom_left_screen_space, top_right_screen_space};
}

Vec2 osc::ui::get_main_window_workspace_dimensions()
{
    return dimensions_of(get_main_window_workspace_ui_rect());
}

float osc::ui::get_main_window_workspace_aspect_ratio()
{
    const ImGuiViewport& viewport = *ImGui::GetMainViewport();
    return aspect_ratio_of(Vec2{viewport.WorkSize});
}

bool osc::ui::is_mouse_in_main_window_workspace()
{
    const Vec2 mousepos = ui::get_mouse_ui_pos();
    const Rect hitRect = get_main_window_workspace_ui_rect();

    return is_intersecting(hitRect, mousepos);
}

bool osc::ui::begin_main_window_top_bar(CStringView label, float height, PanelFlags flags)
{
    return ImGui::BeginViewportSideBar(
        label.c_str(),
        ImGui::GetMainViewport(),
        ImGuiDir_Up,
        height,
        to<ImGuiWindowFlags>(flags)
    );
}

bool osc::ui::begin_main_window_bottom_bar(CStringView label)
{
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
    const float height = ui::get_frame_height() + ui::get_style_panel_padding().y;

    return ImGui::BeginViewportSideBar(
        label.c_str(),
        ImGui::GetMainViewport(),
        ImGuiDir_Down,
        height,
        flags
    );
}

bool osc::ui::draw_button_centered(CStringView label)
{
    const float button_width = ui::calc_text_size(label).x + 2.0f*ui::get_style_frame_padding().x;
    const float midpoint = ui::get_cursor_ui_pos().x + 0.5f*ui::get_content_region_available().x;
    const float button_start_x = midpoint - 0.5f*button_width;

    ui::set_cursor_ui_pos({button_start_x, ui::get_cursor_ui_pos().y});

    return ui::draw_button(label);
}

void osc::ui::draw_text_centered(CStringView content)
{
    const float panel_width = ui::get_panel_size().x;
    const float text_width   = ui::calc_text_size(content).x;

    ui::set_cursor_panel_pos_x(0.5f * (panel_width - text_width));
    draw_text(content);
}

void osc::ui::draw_text_panel_centered(CStringView content)
{
    const auto panel_dimensions = ui::get_panel_size();
    const auto text_dimensions = ui::calc_text_size(content);

    ui::set_cursor_panel_pos(0.5f * (panel_dimensions - text_dimensions));
    draw_text(content);
}

void osc::ui::draw_text_disabled_and_centered(CStringView content)
{
    ui::begin_disabled();
    draw_text_centered(content);
    ui::end_disabled();
}

void osc::ui::draw_text_disabled_and_panel_centered(CStringView content)
{
    ui::begin_disabled();
    draw_text_panel_centered(content);
    ui::end_disabled();
}

void osc::ui::draw_text_column_centered(CStringView content)
{
    const float column_width = ui::get_column_width();
    const float column_offset = ui::get_cursor_panel_pos().x;
    const float text_width = ui::calc_text_size(content).x;

    ui::set_cursor_panel_pos_x(column_offset + 0.5f*(column_width-text_width));
    draw_text(content);
}

void osc::ui::draw_text_faded(CStringView content)
{
    ui::push_style_color(ColorVar::Text, Color::light_grey());
    draw_text(content);
    ui::pop_style_color();
}

void osc::ui::draw_text_warning(CStringView content)
{
    push_style_color(ColorVar::Text, Color::yellow());
    draw_text(content);
    pop_style_color();
}

bool osc::ui::should_save_last_drawn_item_value()
{
    if (ui::is_item_deactivated_after_edit()) {
        return true;  // ImGui detected that the item was deactivated after an edit
    }

    if (ImGui::IsItemEdited() and any_of_keys_pressed({Key::Return, Key::Tab})) {
        return true;  // user pressed enter/tab after editing
    }

    return false;
}

void osc::ui::pop_item_flags(int n)
{
    for (int i = 0; i < n; ++i) {
        ImGui::PopItemFlag();
    }
}

bool osc::ui::draw_combobox(
    CStringView label,
    size_t* current,
    size_t size,
    const std::function<CStringView(size_t)>& accessor)
{
    const CStringView preview = current != nullptr ?
        accessor(*current) :
        CStringView{};

    if (not ui::begin_combobox(label, preview)) {
        return false;
    }

    bool changed = false;
    for (size_t i = 0; i < size; ++i) {
        ui::push_id(static_cast<int>(i));
        const bool is_selected = current != nullptr and *current == i;
        if (ui::draw_selectable(accessor(i), is_selected)) {
            changed = true;
            if (current != nullptr) {
                *current = i;
            }
        }
        if (is_selected) {
            ImGui::SetItemDefaultFocus();
        }
        ui::pop_id();
    }

    ui::end_combobox();

    if (changed) {
        ImGui::MarkItemEdited(ImGui::GetCurrentContext()->LastItemData.ID);
    }

    return changed;
}

bool osc::ui::draw_combobox(
    CStringView label,
    size_t* current,
    std::span<const CStringView> items)
{
    return draw_combobox(
        label,
        current,
        items.size(),
        [items](size_t i) { return items[i]; }
    );
}

void osc::ui::draw_vertical_separator()
{
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
}

void osc::ui::draw_same_line_with_vertical_separator()
{
    ui::same_line();
    draw_vertical_separator();
    ui::same_line();
}



bool osc::ui::draw_float_circular_slider(
    CStringView label,
    float* v,
    float min,
    float max,
    CStringView format,
    SliderFlags flags)
{
    // this implementation was initially copied from `ImGui::SliderFloat` and written in a
    // similar style to code in `imgui_widgets.cpp` (see https://github.com/ocornut/imgui),
    // but has since mutated
    //
    // the display style etc. uses ideas from XEMU (https://github.com/xemu-project/xemu), which
    // contains custom widgets like sliders etc. - I liked XEMU's style but I needed some
    // features that `ImGui::SliderFloat` has, so I ended up with this mash-up

    // prefetch top-level state
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        // skip drawing: the window is not visible or it is clipped
        return false;
    }
    ImGuiContext& g = *ImGui::GetCurrentContext();
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.c_str());

    // calculate top-level item info for early-cull checks etc.
    const Vec2 label_size = ui::calc_text_size(label, true);
    const Vec2 frame_dims = {ImGui::CalcItemWidth(), label_size.y + 2.0f*style.FramePadding.y};
    const Vec2 cursor_screen_pos = ui::get_cursor_ui_pos();
    const ImRect frame_bounds = {cursor_screen_pos, cursor_screen_pos + frame_dims};
    const float label_width_with_spacing = label_size.x > 0.0f ? label_size.x + style.ItemInnerSpacing.x : 0.0f;
    const ImRect total_bounds = {frame_bounds.Min, Vec2{frame_bounds.Max} + Vec2{label_width_with_spacing, 0.0f}};

    const bool temporary_text_input_allowed = (to<ImGuiSliderFlags>(flags) & ImGuiSliderFlags_NoInput) == 0;
    ImGui::ItemSize(total_bounds, style.FramePadding.y);
    if (not ImGui::ItemAdd(total_bounds, id, &frame_bounds, temporary_text_input_allowed ? ImGuiItemFlags_Inputable : 0)) {
        // skip drawing: the slider item is off-screen or not interactable
        return false;
    }
    const bool is_hovered = ImGui::ItemHoverable(frame_bounds, id, g.LastItemData.ItemFlags);  // hovertest the item

    // figure out whether the user is (temporarily) editing the slider as an input text box
    bool temporary_text_input_active = temporary_text_input_allowed and ImGui::TempInputIsActive(id);
    if (not temporary_text_input_active) {
        // tabbing or double-clicking the slider temporarily transforms it into an input box
        const bool clicked = is_hovered and ui::is_mouse_clicked(MouseButton::Left, ui::ID{id});
        const bool double_clicked = (is_hovered and g.IO.MouseClickedCount[0] == 2 and ImGui::TestKeyOwner(ImGuiKey_MouseLeft, id));
        const bool make_active = (clicked or double_clicked or g.NavActivateId == id);

        if (make_active and (clicked or double_clicked)) {
            ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);  // tell ImGui that left-click is locked from further interaction etc. this frame
        }
        if (make_active and temporary_text_input_allowed) {
            if ((clicked and g.IO.KeyCtrl) or double_clicked or (g.NavActivateId == id and (g.NavActivateFlags & ImGuiActivateFlags_PreferInput))) {
                temporary_text_input_active = true;
            }
        }

        // if it's decided that the text input should be made active, then make it active
        // by focusing on it (e.g. give it keyboard focus)
        if (make_active and not temporary_text_input_active) {
            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
            g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
        }
    }

    // if the user is editing the slider as an input text box then draw that instead of the slider
    if (temporary_text_input_active) {
        const bool should_clamp_textual_input = (to<ImGuiSliderFlags>(flags) & ImGuiSliderFlags_AlwaysClamp) != 0;

        return ImGui::TempInputScalar(
            frame_bounds,
            id,
            label.c_str(),
            ImGuiDataType_Float,
            static_cast<void*>(v),
            format.c_str(),
            should_clamp_textual_input ? static_cast<const void*>(&min) : nullptr,
            should_clamp_textual_input ? static_cast<const void*>(&max) : nullptr
        );
    }

    // else: draw the slider (remainder of this func)

    // calculate slider behavior (interaction, etc.)
    //
    // note: I haven't studied `ImGui::SliderBehaviorT` in-depth. I'm just going to
    // go ahead and assume that it's doing the interaction/hittest/mutation logic
    // and leaves rendering to us.
    ImRect grab_bounding_box{};
    const bool value_changed = ImGui::SliderBehaviorT<float, float, float>(
        frame_bounds,
        id,
        ImGuiDataType_Float,
        v,
        min,
        max,
        format.c_str(),
        to<ImGuiSliderFlags>(flags),
        &grab_bounding_box
    );
    if (value_changed) {
        ImGui::MarkItemEdited(id);
    }

    // render
    const bool use_custom_rendering = true;
    if (use_custom_rendering) {
        const Vec2 slider_nob_center = ::centroid_of(grab_bounding_box);
        const float slider_nob_radius = 0.75f * shortest_edge_length_of(grab_bounding_box);
        const float slider_rail_thickness = 0.5f * slider_nob_radius;
        const float slider_rail_top_y = slider_nob_center.y - 0.5f*slider_rail_thickness;
        const float slider_rail_bottom_y = slider_nob_center.y + 0.5f*slider_rail_thickness;

        const bool is_active = g.ActiveId == id;
        const ImU32 rail_color = ImGui::GetColorU32(is_hovered ? ImGuiCol_FrameBgHovered : is_active ? ImGuiCol_FrameBgActive : ImGuiCol_FrameBg);
        const ImU32 grab_color = ImGui::GetColorU32(is_active ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);

        // render left-hand rail (brighter)
        {
            const Vec2 lhs_rail_topleft = {frame_bounds.Min.x, slider_rail_top_y};
            const Vec2 lhs_rail_bottomright = {slider_nob_center.x, slider_rail_bottom_y};
            const ImU32 brightened_rail_color = brighten(rail_color, 2.0f);

            window->DrawList->AddRectFilled(
                lhs_rail_topleft,
                lhs_rail_bottomright,
                brightened_rail_color,
                g.Style.FrameRounding
            );
        }

        // render right-hand rail
        {
            const Vec2 rhs_rail_topleft = {slider_nob_center.x, slider_rail_top_y};
            const Vec2 rhs_rail_bottomright = {frame_bounds.Max.x, slider_rail_bottom_y};

            window->DrawList->AddRectFilled(
                rhs_rail_topleft,
                rhs_rail_bottomright,
                rail_color,
                g.Style.FrameRounding
            );
        }

        // render slider grab on top of rail
        window->DrawList->AddCircleFilled(
            slider_nob_center,
            slider_nob_radius,  // visible nob is slightly smaller than virtual nob
            grab_color
        );

        // render current slider value using user-provided display format
        {
            std::array<char, 64> buf{};
            const char* const buf_end = buf.data() + ImGui::DataTypeFormatString(buf.data(), static_cast<int>(buf.size()), ImGuiDataType_Float, v, format.c_str());
            if (g.LogEnabled) {
                ImGui::LogSetNextTextDecoration("{", "}");
            }
            ImGui::RenderTextClipped(frame_bounds.Min, frame_bounds.Max, buf.data(), buf_end, nullptr, ImVec2(0.5f, 0.5f));
        }

        // render input label in remaining space
        if (label_size.x > 0.0f) {
            ImGui::RenderText(ImVec2(frame_bounds.Max.x + style.ItemInnerSpacing.x, frame_bounds.Min.y + style.FramePadding.y), label.c_str());
        }
    }
    else {
        // render slider background frame
        {
            const ImU32 frame_color = ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : is_hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
            ImGui::RenderNavHighlight(frame_bounds, id);
            ImGui::RenderFrame(frame_bounds.Min, frame_bounds.Max, frame_color, true, g.Style.FrameRounding);
        }

        // render slider grab handle
        if (grab_bounding_box.Max.x > grab_bounding_box.Min.x) {
            window->DrawList->AddRectFilled(grab_bounding_box.Min, grab_bounding_box.Max, ImGui::GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);
        }

        // render current slider value using user-provided display format
        {
            std::array<char, 64> buf{};
            const char* const buf_end = buf.data() + ImGui::DataTypeFormatString(buf.data(), static_cast<int>(buf.size()), ImGuiDataType_Float, v, format.c_str());
            if (g.LogEnabled) {
                ImGui::LogSetNextTextDecoration("{", "}");
            }
            ImGui::RenderTextClipped(frame_bounds.Min, frame_bounds.Max, buf.data(), buf_end, nullptr, ImVec2(0.5f, 0.5f));
        }

        // render input label in remaining space
        if (label_size.x > 0.0f) {
            ImGui::RenderText(ImVec2(frame_bounds.Max.x + style.ItemInnerSpacing.x, frame_bounds.Min.y + style.FramePadding.y), label.c_str());
        }
    }

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));

    return value_changed;
}


// gizmo stuff

bool osc::ui::draw_gizmo_mode_selector(Gizmo& gizmo)
{
    GizmoMode mode = gizmo.mode();
    if (draw_gizmo_mode_selector(mode)) {
        gizmo.set_mode(mode);
        return true;
    }
    return false;
}

bool osc::ui::draw_gizmo_mode_selector(GizmoMode& mode)
{
    constexpr auto mode_labels = std::to_array<CStringView>({ "local", "global" });
    constexpr auto modes = std::to_array<GizmoMode, 2>({ GizmoMode::Local, GizmoMode::World });

    bool rv = false;
    size_t current_mode = std::distance(rgs::begin(modes), rgs::find(modes, mode));
    ui::push_style_var(StyleVar::FrameRounding, 0.0f);
    ui::set_next_item_width(ui::calc_text_size(mode_labels[0]).x + 40.0f);
    if (ui::draw_combobox("##modeselect", &current_mode, mode_labels)) {
        mode = modes.at(current_mode);
        rv = true;
    }
    ui::pop_style_var();
    constexpr CStringView tooltip_title = "Manipulation coordinate system";
    constexpr CStringView tooltip_description = "This affects whether manipulations (such as the arrow gizmos that you can use to translate things) are performed relative to the global coordinate system or the selection's (local) one. Local manipulations can be handy when translating/rotating something that's already rotated.";
    ui::draw_tooltip_if_item_hovered(tooltip_title, tooltip_description);

    return rv;
}

bool osc::ui::draw_gizmo_operation_selector(
    Gizmo& gizmo,
    bool can_translate,
    bool can_rotate,
    bool can_scale,
    CStringView translate_button_text,
    CStringView rotate_button_text,
    CStringView scale_button_text)
{
    GizmoOperation op = gizmo.operation();
    if (draw_gizmo_operation_selector(op, can_translate, can_rotate, can_scale, translate_button_text, rotate_button_text, scale_button_text)) {
        gizmo.set_operation(op);
        return true;
    }
    return false;
}

bool osc::ui::draw_gizmo_operation_selector(
    GizmoOperation& op,
    bool can_translate,
    bool can_rotate,
    bool can_scale,
    CStringView translate_button_text,
    CStringView rotate_button_text,
    CStringView scale_button_text)
{
    bool rv = false;

    ui::push_style_var(StyleVar::ItemSpacing, {0.0f, 0.0f});
    ui::push_style_var(StyleVar::FrameRounding, 0.0f);
    int num_colors_pushed = 0;

    if (can_translate) {
        if (op == GizmoOperation::Translate) {
            ui::push_style_color(ColorVar::Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::draw_button(translate_button_text)) {
            if (op != GizmoOperation::Translate) {
                op = GizmoOperation::Translate;
                rv = true;
            }
        }
        ui::draw_tooltip_if_item_hovered("Translate", "Make the 3D manipulation gizmos translate things (hotkey: G)");
        ui::pop_style_color(std::exchange(num_colors_pushed, 0));
        ui::same_line();
    }

    if (can_rotate) {
        if (op == GizmoOperation::Rotate) {
            ui::push_style_color(ColorVar::Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::draw_button(rotate_button_text)) {
            if (op != GizmoOperation::Rotate) {
                op = GizmoOperation::Rotate;
                rv = true;
            }
        }
        ui::draw_tooltip_if_item_hovered("Rotate", "Make the 3D manipulation gizmos rotate things (hotkey: R)");
        ui::pop_style_color(std::exchange(num_colors_pushed, 0));
        ui::same_line();
    }

    if (can_scale) {
        if (op == GizmoOperation::Scale) {
            ui::push_style_color(ColorVar::Button, Color::muted_blue());
            ++num_colors_pushed;
        }
        if (ui::draw_button(scale_button_text)) {
            if (op != GizmoOperation::Scale) {
                op = GizmoOperation::Scale;
                rv = true;
            }
        }
        ui::draw_tooltip_if_item_hovered("Scale", "Make the 3D manipulation gizmos scale things (hotkey: S)");
        ui::pop_style_color(std::exchange(num_colors_pushed, 0));
        ui::same_line();
    }

    ui::pop_style_var(2);

    return rv;
}

std::optional<Transform> osc::ui::Gizmo::draw(
    Mat4& model_matrix,
    const Mat4& view_matrix,
    const Mat4& projection_matrix,
    const Rect& ui_rect)
{
    return draw_to(
        model_matrix,
        view_matrix,
        projection_matrix,
        ui_rect,
        ImGui::GetWindowDrawList()
    );
}

std::optional<Transform> osc::ui::Gizmo::draw_to_foreground(
    Mat4& model_matrix,
    const Mat4& view_matrix,
    const Mat4& projection_matrix,
    const Rect& ui_rect)
{
    return draw_to(
        model_matrix,
        view_matrix,
        projection_matrix,
        ui_rect,
        ImGui::GetForegroundDrawList()
    );
}

std::optional<Transform> osc::ui::Gizmo::draw_to(
    Mat4& model_matrix,
    const Mat4& view_matrix,
    const Mat4& projection_matrix,
    const Rect& ui_rect,
    ImDrawList* draw_list)
{
    if (operation_ == GizmoOperation::None) {
        return std::nullopt;  // disabled
    }

    // important: necessary when showing multiple gizmos in one frame
    // also important: don't use ui::get_id(), because it uses an ID stack and we might want to know if "isover" etc. is true outside of a window
    ImGuizmo::PushID(id_);
    const ScopeExit g{[]{ ImGuizmo::PopID(); }};

    // update last-frame cache
    was_using_last_frame_ = ImGuizmo::IsUsing();

    ImGuizmo::SetRect(
        ui_rect.p1.x,
        ui_rect.p1.y,
        dimensions_of(ui_rect).x,
        dimensions_of(ui_rect).y
    );
    ImGuizmo::SetDrawlist(draw_list);

    // use rotation from the parent, translation from station
    Mat4 delta_matrix;

    const bool gizmo_was_manipulated_by_user = ImGuizmo::Manipulate(
        value_ptr(view_matrix),
        value_ptr(projection_matrix),
        to<ImGuizmo::OPERATION>(operation_),
        to<ImGuizmo::MODE>(mode_),
        value_ptr(model_matrix),
        value_ptr(delta_matrix),
        nullptr,
        nullptr,
        nullptr
    );

    if (not gizmo_was_manipulated_by_user) {
        return std::nullopt;  // user is not interacting, so no changes to apply
    }

    return decompose_to_transform(delta_matrix);
}

bool osc::ui::Gizmo::is_using() const
{
    ImGuizmo::PushID(id_);
    const bool rv = ImGuizmo::IsUsing();
    ImGuizmo::PopID();
    return rv;
}

bool osc::ui::Gizmo::is_over() const
{
    ImGuizmo::PushID(id_);
    const bool rv = ImGuizmo::IsOver();
    ImGuizmo::PopID();
    return rv;
}

bool osc::ui::Gizmo::handle_keyboard_inputs()
{
    bool const shift_down = ui::is_shift_down();
    bool const ctrl_or_super_down = ui::is_ctrl_or_super_down();

    if (shift_down or ctrl_or_super_down) {
        return false;  // assume the user is doing some other action
    }
    else if (ui::is_key_pressed(Key::R)) {

        // R: set manipulation mode to "rotate"
        if (operation_ == GizmoOperation::Rotate) {
            mode_ = mode_ == GizmoMode::Local ? GizmoMode::World : GizmoMode::Local;
        }
        operation_ = GizmoOperation::Rotate;
        return true;
    }
    else if (ui::is_key_pressed(Key::G)) {

        // G: set manipulation mode to "grab" (translate)
        if (operation_ == GizmoOperation::Translate) {
            mode_ = mode_ == GizmoMode::Local ? GizmoMode::World : GizmoMode::Local;
        }
        operation_ = GizmoOperation::Translate;
        return true;
    }
    else if (ui::is_key_pressed(Key::S)) {

        // S: set manipulation mode to "scale"
        if (operation_ == GizmoOperation::Scale) {
            mode_ = mode_ == GizmoMode::Local ? GizmoMode::World : GizmoMode::Local;
        }
        operation_ = GizmoOperation::Scale;
        return true;
    }
    else {
        return false;
    }
}

// `ui::plot::` helpers
namespace
{
    constexpr ImPlotFlags to_ImPlotFlags(plot::PlotFlags flags)
    {
        static_assert(std::to_underlying(plot::PlotFlags::NoTitle) == ImPlotFlags_NoTitle);
        static_assert(std::to_underlying(plot::PlotFlags::NoLegend) == ImPlotFlags_NoLegend);
        static_assert(std::to_underlying(plot::PlotFlags::NoMenus) == ImPlotFlags_NoMenus);
        static_assert(std::to_underlying(plot::PlotFlags::NoBoxSelect) == ImPlotFlags_NoBoxSelect);
        static_assert(std::to_underlying(plot::PlotFlags::NoFrame) == ImPlotFlags_NoFrame);
        static_assert(std::to_underlying(plot::PlotFlags::NoInputs) == ImPlotFlags_NoInputs);
        return static_cast<ImPlotFlags>(flags);
    }

    constexpr ImPlotStyleVar to_ImPlotStyleVar(plot::PlotStyleVar var)
    {
        static_assert(num_options<plot::PlotStyleVar>() == 4);
        switch (var) {
        case plot::PlotStyleVar::FitPadding:        return ImPlotStyleVar_FitPadding;
        case plot::PlotStyleVar::PlotPadding:       return ImPlotStyleVar_PlotPadding;
        case plot::PlotStyleVar::PlotBorderSize:    return ImPlotStyleVar_PlotBorderSize;
        case plot::PlotStyleVar::AnnotationPadding: return ImPlotStyleVar_AnnotationPadding;
        default:                                    std::unreachable();
        }
    }

    constexpr ImPlotCol to_ImPlotCol(plot::PlotColorVar var)
    {
        static_assert(num_options<plot::PlotColorVar>() == 2);
        switch (var) {
        case plot::PlotColorVar::Line:           return ImPlotCol_Line;
        case plot::PlotColorVar::PlotBackground: return ImPlotCol_PlotBg;
        default:                                 std::unreachable();
        }
    }

    constexpr ImAxis to_ImAxis(plot::Axis axis)
    {
        static_assert(num_options<plot::Axis>() == 2);
        switch (axis) {
        case plot::Axis::X1: return ImAxis_X1;
        case plot::Axis::Y1: return ImAxis_Y1;
        default:             std::unreachable();
        }
    }

    constexpr ImPlotAxisFlags to_ImPlotAxisFlags(plot::AxisFlags flags)
    {
        static_assert(std::to_underlying(plot::AxisFlags::None) == ImPlotAxisFlags_None);
        static_assert(std::to_underlying(plot::AxisFlags::NoLabel) == ImPlotAxisFlags_NoLabel);
        static_assert(std::to_underlying(plot::AxisFlags::NoGridLines) == ImPlotAxisFlags_NoGridLines);
        static_assert(std::to_underlying(plot::AxisFlags::NoTickMarks) == ImPlotAxisFlags_NoTickMarks);
        static_assert(std::to_underlying(plot::AxisFlags::NoTickLabels) == ImPlotAxisFlags_NoTickLabels);
        static_assert(std::to_underlying(plot::AxisFlags::NoMenus) == ImPlotAxisFlags_NoMenus);
        static_assert(std::to_underlying(plot::AxisFlags::AutoFit) == ImPlotAxisFlags_AutoFit);
        static_assert(std::to_underlying(plot::AxisFlags::LockMin) == ImPlotAxisFlags_LockMin);
        static_assert(std::to_underlying(plot::AxisFlags::LockMax) == ImPlotAxisFlags_LockMax);
        static_assert(std::to_underlying(plot::AxisFlags::Lock) == ImPlotAxisFlags_Lock);
        static_assert(std::to_underlying(plot::AxisFlags::NoDecorations) == ImPlotAxisFlags_NoDecorations);

        return static_cast<ImPlotAxisFlags>(flags);
    }

    constexpr ImPlotCond to_ImPlotCond(plot::Condition condition)
    {
        static_assert(num_options<plot::Condition>() == 2);
        switch (condition) {
        case plot::Condition::Always: return ImPlotCond_Always;
        case plot::Condition::Once:   return ImPlotCond_Once;
        default:                      std::unreachable();
        }
    }

    constexpr ImPlotMarker to_ImPlotMarker(plot::MarkerType marker_type)
    {
        static_assert(num_options<plot::MarkerType>() == 2);
        switch (marker_type) {
        case plot::MarkerType::None:   return ImPlotMarker_None;
        case plot::MarkerType::Circle: return ImPlotMarker_Circle;
        default:                       std::unreachable();
        }
    }

    constexpr ImPlotDragToolFlags to_ImPlotDragToolFlags(plot::DragToolFlags flags)
    {
        static_assert(std::to_underlying(plot::DragToolFlag::None) == ImPlotDragToolFlags_None);
        static_assert(std::to_underlying(plot::DragToolFlag::NoFit) == ImPlotDragToolFlags_NoFit);
        static_assert(std::to_underlying(plot::DragToolFlag::NoInputs) == ImPlotDragToolFlags_NoInputs);
        static_assert(num_flags<plot::DragToolFlag>() == 2);
        return static_cast<ImPlotDragToolFlags>(flags.underlying_value());
    }

    constexpr ImPlotLocation to_ImPlotLocation(plot::Location location)
    {
        static_assert(num_options<plot::Location>() == 9);
        switch (location) {
        case plot::Location::Center:    return ImPlotLocation_Center;
        case plot::Location::North:     return ImPlotLocation_North;
        case plot::Location::NorthEast: return ImPlotLocation_NorthEast;
        case plot::Location::East:      return ImPlotLocation_East;
        case plot::Location::SouthEast: return ImPlotLocation_SouthEast;
        case plot::Location::South:     return ImPlotLocation_South;
        case plot::Location::SouthWest: return ImPlotLocation_SouthWest;
        case plot::Location::West:      return ImPlotLocation_West;
        case plot::Location::NorthWest: return ImPlotLocation_NorthWest;
        default:                        std::unreachable();
        }
    }

    constexpr ImPlotLegendFlags to_ImPlotLegendFlags(plot::LegendFlags flags)
    {
        static_assert(std::to_underlying(plot::LegendFlags::None) == ImPlotLegendFlags_None);
        static_assert(std::to_underlying(plot::LegendFlags::Outside) == ImPlotLegendFlags_Outside);
        return static_cast<ImPlotLegendFlags>(flags);
    }
}

void osc::ui::plot::show_demo_panel()
{
    ImPlot::ShowDemoWindow();
}

bool osc::ui::plot::begin(CStringView title, Vec2 size, PlotFlags flags)
{
    return ImPlot::BeginPlot(title.c_str(), size, to_ImPlotFlags(flags));
}

void osc::ui::plot::end()
{
    ImPlot::EndPlot();
}

void osc::ui::plot::push_style_var(PlotStyleVar var, float value)
{
    ImPlot::PushStyleVar(to_ImPlotStyleVar(var), value);
}

void osc::ui::plot::push_style_var(PlotStyleVar var, Vec2 value)
{
    ImPlot::PushStyleVar(to_ImPlotStyleVar(var), value);
}

void osc::ui::plot::pop_style_var(int count)
{
    ImPlot::PopStyleVar(count);
}

void osc::ui::plot::push_style_color(PlotColorVar var, const Color& color)
{
    ImPlot::PushStyleColor(to_ImPlotCol(var), color);
}

void osc::ui::plot::pop_style_color(int count)
{
    ImPlot::PopStyleColor(count);
}

void osc::ui::plot::setup_axis(Axis axis, std::optional<CStringView> label, AxisFlags flags)
{
    ImPlot::SetupAxis(to_ImAxis(axis), label ? label->c_str() : nullptr, to_ImPlotAxisFlags(flags));
}

void osc::ui::plot::setup_axes(CStringView x_label, CStringView y_label, AxisFlags x_flags, AxisFlags y_flags)
{
    ImPlot::SetupAxes(x_label.c_str(), y_label.c_str(), to_ImPlotAxisFlags(x_flags), to_ImPlotAxisFlags(y_flags));
}

void osc::ui::plot::setup_axis_limits(Axis axis, ClosedInterval<float> data_range, float padding_percentage, Condition condition)
{
    // apply padding
    data_range = expand_by_absolute_amount(data_range, padding_percentage * data_range.half_length());

    // apply absolute padding in the edge-case where the data is constant
    if (equal_within_scaled_epsilon(data_range.lower, data_range.upper)) {
        data_range = expand_by_absolute_amount(data_range, 0.5f);
    }

    ImPlot::SetupAxisLimits(to_ImAxis(axis), data_range.lower, data_range.upper, to_ImPlotCond(condition));
}

void osc::ui::plot::setup_finish()
{
    ImPlot::SetupFinish();
}

void osc::ui::plot::set_next_marker_style(
    MarkerType marker_type,
    std::optional<float> size,
    std::optional<Color> fill,
    std::optional<float> weight,
    std::optional<Color> outline)
{
    ImPlot::SetNextMarkerStyle(
        to_ImPlotMarker(marker_type),
        size ? *size : IMPLOT_AUTO,
        fill ? ImVec4{*fill} : IMPLOT_AUTO_COL,
        weight ? *weight : IMPLOT_AUTO,
        outline ? ImVec4{*outline} : IMPLOT_AUTO_COL
    );
}

void osc::ui::plot::plot_line(CStringView name, std::span<const Vec2> points)
{
    ImPlot::PlotLine(
        name.c_str(),
        points.empty() ? nullptr : &points.front().x,
        points.empty() ? nullptr : &points.front().y,
        static_cast<int>(points.size()),
        0,
        0,
        sizeof(Vec2)
    );
}

void osc::ui::plot::plot_line(CStringView name, std::span<const float> points)
{
    ImPlot::PlotLine(name.c_str(), points.data(), static_cast<int>(points.size()));
}

Rect osc::ui::plot::get_plot_ui_rect()
{
    const Vec2 top_left = ImPlot::GetPlotPos();
    return {top_left, top_left + Vec2{ImPlot::GetPlotSize()}};
}

void osc::ui::plot::detail::draw_annotation_v(Vec2 location_dataspace, const Color& color, Vec2 pixel_offset, bool clamp, CStringView fmt, va_list args)
{
    ImPlot::AnnotationV(location_dataspace.x, location_dataspace.y, color, pixel_offset, clamp, fmt.c_str(), args);
}

bool osc::ui::plot::drag_point(int id, Vec2d* plot_point, const Color& color, float size, DragToolFlags flags)
{
    return ImPlot::DragPoint(id, &plot_point->x, &plot_point->y, color, size, to_ImPlotDragToolFlags(flags));
}

bool osc::ui::plot::drag_line_x(int id, double* plot_x, const Color& color, float thickness, DragToolFlags flags)
{
    return ImPlot::DragLineX(id, plot_x, color, thickness, to_ImPlotDragToolFlags(flags));
}

bool osc::ui::plot::drag_line_y(int id, double* plot_y, const Color& color, float thickness, DragToolFlags flags)
{
    return ImPlot::DragLineY(id, plot_y, color, thickness, to_ImPlotDragToolFlags(flags));
}

void osc::ui::plot::tag_x(double plot_x, const Color& color, bool round)
{
    ImPlot::TagX(plot_x, color, round);
}

bool osc::ui::plot::is_plot_hovered()
{
    return ImPlot::IsPlotHovered();
}

Vec2 osc::ui::plot::get_plot_mouse_pos()
{
    const auto pos = ImPlot::GetPlotMousePos();
    return Vec2{pos.x, pos.y};
}

Vec2 osc::ui::plot::get_plot_mouse_pos(Axis x_axis, Axis y_axis)
{
    const auto pos = ImPlot::GetPlotMousePos(to_ImAxis(x_axis), to_ImAxis(y_axis));
    return Vec2{pos.x, pos.y};
}

void osc::ui::plot::setup_legend(Location location, LegendFlags flags)
{
    ImPlot::SetupLegend(to_ImPlotLocation(location), to_ImPlotLegendFlags(flags));
}

bool osc::ui::plot::begin_legend_popup(CStringView label_id, MouseButton mouse_button)
{
    return ImPlot::BeginLegendPopup(label_id.c_str(), to<ImGuiMouseButton>(mouse_button));
}

void osc::ui::plot::end_legend_popup()
{
    ImPlot::EndLegendPopup();
}

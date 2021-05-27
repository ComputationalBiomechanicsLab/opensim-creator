#pragma once

#include "src/3d/3d.hpp"
#include "src/resources.hpp"

/**
 * what you are about to see (using SFINAE to test whether a class has a texcoord member)
 * is better described with a diagram:

        _            _.,----,
__  _.-._ / '-.        -  ,._  \)
|  `-)_   '-.   \       / < _ )/" }
/__    '-.   \   '-, ___(c-(6)=(6)
, `'.    `._ '.  _,'   >\    "  )
:;;,,'-._   '---' (  ( "/`. -='/
;:;;:;;,  '..__    ,`-.`)'- '--'
;';:;;;;;'-._ /'._|   Y/   _/' \
  '''"._ F    |  _/ _.'._   `\
         L    \   \/     '._  \
  .-,-,_ |     `.  `'---,  \_ _|
  //    'L    /  \,   ("--',=`)7
 | `._       : _,  \  /'`-._L,_'-._
 '--' '-.\__/ _L   .`'         './/
             [ (  /
              ) `{
   snd        \__)

 */
template<typename>
struct sfinae_true : std::true_type {};
namespace detail {
    template<typename T>
    static auto test_has_texcoord(int) -> sfinae_true<decltype(std::declval<T>().texcoord)>;
    template<typename T>
    static auto test_has_texcoord(long) -> std::false_type;
}
template<typename T>
struct has_texcoord : decltype(detail::test_has_texcoord<T>(0)) {};

namespace osc {
    // An instanced multi-render-target (MRT) shader that performes Gouraud shading for
    // COLOR0 and RGB passthrough for COLOR1
    //
    // - COLOR0: geometry colored with Gouraud shading: i.e. "the scene"
    // - COLOR1: RGB passthrough (selection logic + rim alphas)
    struct Gouraud_mrt_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(slurp_resource("shaders/gouraud_mrt.vert")),
            gl::CompileFromSource<gl::Fragment_shader>(slurp_resource("shaders/gouraud_mrt.frag")));

        // vertex attrs
        static constexpr gl::Attribute_vec3 aLocation{0};
        static constexpr gl::Attribute_vec3 aNormal{1};
        static constexpr gl::Attribute_vec2 aTexCoord{2};

        // instancing attrs
        static constexpr gl::Attribute_mat4x3 aModelMat{3};
        static constexpr gl::Attribute_mat3 aNormalMat{7};
        static constexpr gl::Attribute_vec4 aRgba0{10};
        static constexpr gl::Attribute_vec3 aRgb1{11};

        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(program, "uProjMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(program, "uViewMat");
        gl::Uniform_vec3 uLightDir = gl::GetUniformLocation(program, "uLightDir");
        gl::Uniform_vec3 uLightColor = gl::GetUniformLocation(program, "uLightColor");
        gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(program, "uViewPos");
        gl::Uniform_bool uIsTextured = gl::GetUniformLocation(program, "uIsTextured");
        gl::Uniform_bool uIsShaded = gl::GetUniformLocation(program, "uIsShaded");
        gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(program, "uSampler0");
        gl::Uniform_bool uSkipVP = gl::GetUniformLocation(program, "uSkipVP");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(
            Vbo& vbo,
            gl::Typed_buffer_handle<GL_ELEMENT_ARRAY_BUFFER>& ebo,
            gl::Array_buffer<Mesh_instance, GL_DYNAMIC_DRAW>& instance_vbo) {

            static_assert(std::is_standard_layout<T>::value, "this is required for offsetof macro usage");

            gl::Vertex_array vao;

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aLocation, false, sizeof(T), offsetof(T, pos));
            gl::EnableVertexAttribArray(aLocation);
            gl::VertexAttribPointer(aNormal, false, sizeof(T), offsetof(T, normal));
            gl::EnableVertexAttribArray(aNormal);

            if constexpr (has_texcoord<T>::value) {
                gl::VertexAttribPointer(aTexCoord, false, sizeof(T), offsetof(T, texcoord));
                gl::EnableVertexAttribArray(aTexCoord);
            }

            gl::BindBuffer(ebo.buffer_type, ebo);

            // set up instanced VBOs
            gl::BindBuffer(instance_vbo);

            gl::VertexAttribPointer(aModelMat, false, sizeof(Mesh_instance), offsetof(Mesh_instance, model_xform));
            gl::EnableVertexAttribArray(aModelMat);
            gl::VertexAttribDivisor(aModelMat, 1);

            gl::VertexAttribPointer(aNormalMat, false, sizeof(Mesh_instance), offsetof(Mesh_instance, normal_xform));
            gl::EnableVertexAttribArray(aNormalMat);
            gl::VertexAttribDivisor(aNormalMat, 1);

            // note: RGBs are tricksy, because their CPU-side data is UNSIGNED_BYTEs but
            // their GPU-side data is normalized FLOATs

            gl::VertexAttribPointer<decltype(aRgba0)::glsl_type, GL_UNSIGNED_BYTE>(
                aRgba0, true, sizeof(Mesh_instance), offsetof(Mesh_instance, rgba));
            gl::EnableVertexAttribArray(aRgba0);
            gl::VertexAttribDivisor(aRgba0, 1);

            gl::VertexAttribPointer<decltype(aRgb1)::glsl_type, GL_UNSIGNED_BYTE>(
                aRgb1, true, sizeof(Mesh_instance), offsetof(Mesh_instance, passthrough));
            gl::EnableVertexAttribArray(aRgb1);
            gl::VertexAttribDivisor(aRgb1, 1);

            gl::BindVertexArray();

            return vao;
        }
    };

    // A basic shader that just samples a texture onto the provided geometry
    //
    // useful for rendering quads etc.
    struct Colormapped_plain_texture_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(slurp_resource("shaders/colormapped_plain_texture.vert")),
            gl::CompileFromSource<gl::Fragment_shader>(slurp_resource("shaders/colormapped_plain_texture.frag")));

        static constexpr gl::Attribute_vec3 aPos{0};
        static constexpr gl::Attribute_vec2 aTexCoord{1};

        gl::Uniform_mat4 uMVP = gl::GetUniformLocation(p, "uMVP");
        gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");
        gl::Uniform_mat4 uSamplerMultiplier = gl::GetUniformLocation(p, "uSamplerMultiplier");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao;

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoord, false, sizeof(T), offsetof(T, texcoord));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            return vao;
        }
    };

    struct Plain_texture_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(slurp_resource("shaders/plain_texture.vert")),
            gl::CompileFromSource<gl::Fragment_shader>(slurp_resource("shaders/plain_texture.frag")));

        static constexpr gl::Attribute_vec3 aPos{0};
        static constexpr gl::Attribute_vec2 aTexCoord{1};

        gl::Uniform_mat4 uMVP = gl::GetUniformLocation(p, "uMVP");
        gl::Uniform_float uTextureScaler = gl::GetUniformLocation(p, "uTextureScaler");
        gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao;

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoord, false, sizeof(T), offsetof(T, texcoord));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            return vao;
        }
    };

    // A specialized edge-detection shader for rim highlighting
    struct Edge_detection_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(slurp_resource("shaders/edge_detect.vert")),
            gl::CompileFromSource<gl::Fragment_shader>(slurp_resource("shaders/edge_detect.frag")));

        static constexpr gl::Attribute_vec3 aPos{0};
        static constexpr gl::Attribute_vec2 aTexCoord{1};

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(p, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(p, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(p, "uProjMat");
        gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");
        gl::Uniform_vec4 uRimRgba = gl::GetUniformLocation(p, "uRimRgba");
        gl::Uniform_float uRimThickness = gl::GetUniformLocation(p, "uRimThickness");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao;

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoord, false, sizeof(T), offsetof(T, texcoord));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            return vao;
        }
    };

    struct Skip_msxaa_blitter_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(slurp_resource("shaders/skip_msxaa_blitter.vert")),
            gl::CompileFromSource<gl::Fragment_shader>(slurp_resource("shaders/skip_msxaa_blitter.frag")));

        static constexpr gl::Attribute_vec3 aPos{0};
        static constexpr gl::Attribute_vec2 aTexCoord{1};

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(p, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(p, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(p, "uProjMat");
        gl::Uniform_sampler2DMS uSampler0 = gl::GetUniformLocation(p, "uSampler0");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao;

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aTexCoord, false, sizeof(T), offsetof(T, texcoord));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            return vao;
        }
    };

    // uses a geometry shader to render normals as lines
    struct Normals_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::CompileFromSource<gl::Vertex_shader>(slurp_resource("shaders/draw_normals.vert")),
            gl::CompileFromSource<gl::Fragment_shader>(slurp_resource("shaders/draw_normals.frag")),
            gl::CompileFromSource<gl::Geometry_shader>(slurp_resource("shaders/draw_normals.geom")));

        static constexpr gl::Attribute_vec3 aPos{0};
        static constexpr gl::Attribute_vec3 aNormal{1};

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(program, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(program, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(program, "uProjMat");
        gl::Uniform_mat4 uNormalMat = gl::GetUniformLocation(program, "uNormalMat");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao;
            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, false, sizeof(T), offsetof(T, pos));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(aNormal, false, sizeof(T), offsetof(T, normal));
            gl::EnableVertexAttribArray(aNormal);
            gl::BindVertexArray();
            return vao;
        }
    };
}

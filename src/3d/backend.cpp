// backend: implements the functionality exposed by all the other headers
// in 3d/

#include "src/3d/drawlist.hpp"
#include "src/3d/gl.hpp"
#include "src/3d/gpu_data_reference.hpp"
#include "src/3d/gpu_storage.hpp"
#include "src/3d/mesh_generation.hpp"
#include "src/3d/mesh_instance.hpp"
#include "src/3d/mesh_storage.hpp"
#include "src/3d/render_target.hpp"
#include "src/3d/renderer.hpp"
#include "src/3d/shader_cache.hpp"
#include "src/3d/texture_storage.hpp"
#include "src/3d/textured_vert.hpp"
#include "src/3d/untextured_vert.hpp"
#include "src/config.hpp"

#include <GL/glew.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>
#include <vector>

using namespace osmv;

namespace {
    // These static asserts are used in order to catch some basic low-level footguns
    static_assert(
        std::is_trivially_copyable<Mesh_instance>::value,
        "a mesh instance should ideally be trivially copyable, because it is potentially used *a lot* in draw calls. You can remove this assert if you disagree");
    static_assert(
        std::is_standard_layout<Mesh_instance>::value,
        "this is required for offsetof macro usage, which is used for setting up OpenGL attribute pointers. Review `VertexAttribPointer` calls. See: https://en.cppreference.com/w/cpp/types/is_standard_layout");
    static_assert(
        std::is_trivially_destructible<Mesh_instance>::value,
        "this is a purely a nice-to-have. It enables bulk-destroying mesh instances in a collection class with zero overhead");

    static_assert(
        std::is_trivially_copyable<Mesh_reference>::value,
        "a mesh reference should be a trivially copyable type (e.g. a number), because it is used in *a lot* of places");
    static_assert(std::is_trivially_move_assignable<Mesh_reference>::value);
    static_assert(std::is_trivially_move_constructible<Mesh_reference>::value);
    static_assert(
        std::is_trivially_destructible<Mesh_reference>::value,
        "a mesh reference should be trivially destructible, because it is used in *a lot* of places and ends up in collections etc.");
    static_assert(std::is_standard_layout<osmv::Mesh_reference>::value);
    static_assert(
        std::has_unique_object_representations<Mesh_reference>::value,
        "i.e. is comparing the bytes of the object sufficient enough to implement equality for the object. This is a nice-to-have and comes in handy when bulk hashing instance ids");

    static_assert(
        sizeof(Textured_vert) == 8 * sizeof(float),
        "unexpected struct size: could cause problems when uploading to the GPU: review where this is used");

    static_assert(
        sizeof(glm::vec3) == 3 * sizeof(float),
        "unexpected struct size: could cause problems when uploading to the GPU: review where this is used");
    static_assert(
        sizeof(osmv::Untextured_vert) == 6 * sizeof(float),
        "unexpected struct size: could cause problems when uploading to the GPU: review where this is used");

    void Mat4Pointer(gl::Attribute const& mat4loc, size_t base_offset) {
        GLuint loc = static_cast<GLuint>(mat4loc);
        for (unsigned i = 0; i < 4; ++i) {
            // HACK: from LearnOpenGL: mat4's must be set in this way because
            //       of OpenGL not allowing more than 4 or so floats to be set
            //       in a single call
            //
            // see:
            // https://learnopengl.com/code_viewer_gh.php?code=src/4.advanced_opengl/10.3.asteroids_instanced/asteroids_instanced.cpp
            glVertexAttribPointer(
                loc + i,
                4,
                GL_FLOAT,
                GL_FALSE,
                sizeof(osmv::Mesh_instance),
                reinterpret_cast<void*>(base_offset + i * sizeof(glm::vec4)));
            glEnableVertexAttribArray(loc + i);
            glVertexAttribDivisor(loc + i, 1);
        }
    }

    void Mat4x3Pointer(gl::Attribute const& mat4x3loc, size_t base_offset) {
        GLuint loc = static_cast<GLuint>(mat4x3loc);
        for (unsigned i = 0; i < 4; ++i) {
            // HACK: from LearnOpenGL: mat4's must be set in this way because
            //       of OpenGL not allowing more than 4 or so floats to be set
            //       in a single call
            //
            // see:
            // https://learnopengl.com/code_viewer_gh.php?code=src/4.advanced_opengl/10.3.asteroids_instanced/asteroids_instanced.cpp
            glVertexAttribPointer(
                loc + i,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(osmv::Mesh_instance),
                reinterpret_cast<void*>(base_offset + i * sizeof(glm::vec3)));
            glEnableVertexAttribArray(loc + i);
            glVertexAttribDivisor(loc + i, 1);
        }
    }

    void Mat3Pointer(gl::Attribute const& mat3loc, size_t base_offset) {
        GLuint loc = static_cast<GLuint>(mat3loc);
        for (unsigned i = 0; i < 3; ++i) {
            // HACK: from LearnOpenGL: mat4's must be set in this way because
            //       of OpenGL not allowing more than 4 or so floats to be set
            //       in a single call
            //
            // see:
            // https://learnopengl.com/code_viewer_gh.php?code=src/4.advanced_opengl/10.3.asteroids_instanced/asteroids_instanced.cpp
            glVertexAttribPointer(
                loc + i,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(osmv::Mesh_instance),
                reinterpret_cast<void*>(base_offset + i * sizeof(glm::vec3)));
            glEnableVertexAttribArray(loc + i);
            glVertexAttribDivisor(loc + i, 1);
        }
    }

    void Vec4Pointer(gl::Attribute const& vec4log, size_t base_offset) {
        glVertexAttribPointer(
            vec4log, 4, GL_FLOAT, GL_FALSE, sizeof(osmv::Mesh_instance), reinterpret_cast<void*>(base_offset));
        glEnableVertexAttribArray(vec4log);
        glVertexAttribDivisor(vec4log, 1);
    }

    void u8_to_Vec3Pointer(gl::Attribute const& vec3log, size_t base_offset) {
        glVertexAttribPointer(
            vec3log, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(osmv::Mesh_instance), reinterpret_cast<void*>(base_offset));
        glEnableVertexAttribArray(vec3log);
        glVertexAttribDivisor(vec3log, 1);
    }

    void u8_to_Vec4Pointer(gl::Attribute const& vec4log, size_t base_offset) {
        glVertexAttribPointer(
            vec4log, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(osmv::Mesh_instance), reinterpret_cast<void*>(base_offset));
        glEnableVertexAttribArray(vec4log);
        glVertexAttribDivisor(vec4log, 1);
    }

    /**
     * what you are about to see (using SFINAE to test whether a class has a uv coord)
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

    // An instanced multi-render-target (MRT) shader that performes Gouraud shading for
    // COLOR0 and RGB passthrough for COLOR1
    //
    // - COLOR0: geometry colored with Gouraud shading: i.e. "the scene"
    // - COLOR1: RGB passthrough (selection logic + rim alphas)
    struct Gouraud_mrt_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::config::shader_path("gouraud_mrt.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::config::shader_path("gouraud_mrt.frag")));

        // vertex attrs
        static constexpr gl::Attribute aLocation = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);
        static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(2);

        // instancing attrs
        static constexpr gl::Attribute aModelMat = gl::AttributeAtLocation(3);
        static constexpr gl::Attribute aNormalMat = gl::AttributeAtLocation(7);
        static constexpr gl::Attribute aRgba0 = gl::AttributeAtLocation(10);
        static constexpr gl::Attribute aRgb1 = gl::AttributeAtLocation(11);

        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(program, "uProjMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(program, "uViewMat");
        gl::Uniform_vec3 uLightPos = gl::GetUniformLocation(program, "uLightPos");
        gl::Uniform_vec3 uLightColor = gl::GetUniformLocation(program, "uLightColor");
        gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(program, "uViewPos");
        gl::Uniform_bool uIsTextured = gl::GetUniformLocation(program, "uIsTextured");
        gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(program, "uSampler0");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo, gl::Array_bufferT<osmv::Mesh_instance>& instance_vbo) {
            static_assert(std::is_standard_layout<T>::value, "this is required for offsetof macro usage");

            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(
                aLocation, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aLocation);
            gl::VertexAttribPointer(
                aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, normal)));
            gl::EnableVertexAttribArray(aNormal);

            if constexpr (has_texcoord<T>::value) {
                gl::VertexAttribPointer(
                    aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, texcoord)));
                gl::EnableVertexAttribArray(aTexCoord);
            }

            gl::BindBuffer(instance_vbo);
            Mat4x3Pointer(aModelMat, offsetof(osmv::Mesh_instance, transform));
            Mat3Pointer(aNormalMat, offsetof(osmv::Mesh_instance, _normal_xform));
            u8_to_Vec4Pointer(aRgba0, offsetof(osmv::Mesh_instance, rgba));
            u8_to_Vec3Pointer(aRgb1, offsetof(osmv::Mesh_instance, _passthrough));

            gl::BindVertexArray();

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();

            return vao;
        }
    };

    // A basic shader that just samples a texture onto the provided geometry
    //
    // useful for rendering quads etc.
    struct Colormapped_plain_texture_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::config::shader_path("colormapped_plain_texture.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::config::shader_path("colormapped_plain_texture.frag")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 uMVP = gl::GetUniformLocation(p, "uMVP");
        gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");
        gl::Uniform_mat4 uSamplerMultiplier = gl::GetUniformLocation(p, "uSamplerMultiplier");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(
                aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, texcoord)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();

            return vao;
        }
    };

    //
    // useful for rendering quads etc.
    struct Plain_texture_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::config::shader_path("plain_texture.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::config::shader_path("plain_texture.frag")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 uMVP = gl::GetUniformLocation(p, "uMVP");
        gl::Uniform_float uTextureScaler = gl::GetUniformLocation(p, "uTextureScaler");
        gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(
                aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, texcoord)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();

            return vao;
        }
    };

    // A specialized edge-detection shader for rim highlighting
    struct Edge_detection_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::config::shader_path("edge_detect.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::config::shader_path("edge_detect.frag")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(p, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(p, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(p, "uProjMat");
        gl::Uniform_sampler2d uSampler0 = gl::GetUniformLocation(p, "uSampler0");
        gl::Uniform_vec4 uRimRgba = gl::GetUniformLocation(p, "uRimRgba");
        gl::Uniform_float uRimThickness = gl::GetUniformLocation(p, "uRimThickness");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(
                aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, texcoord)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();

            return vao;
        }
    };

    struct Skip_msxaa_blitter_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::config::shader_path("skip_msxaa_blitter.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::config::shader_path("skip_msxaa_blitter.frag")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(p, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(p, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(p, "uProjMat");
        gl::Uniform_sampler2DMS uSampler0 = gl::GetUniformLocation(p, "uSampler0");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(
                aTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, texcoord)));
            gl::EnableVertexAttribArray(aTexCoord);
            gl::BindVertexArray();

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();

            return vao;
        }
    };

    // uses a geometry shader to render normals as lines
    struct Normals_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::config::shader_path("draw_normals.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::config::shader_path("draw_normals.frag")),
            gl::Compile<gl::Geometry_shader>(osmv::config::shader_path("draw_normals.geom")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 uModelMat = gl::GetUniformLocation(program, "uModelMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(program, "uViewMat");
        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(program, "uProjMat");
        gl::Uniform_mat4 uNormalMat = gl::GetUniformLocation(program, "uNormalMat");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();
            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aPos);
            gl::VertexAttribPointer(
                aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, normal)));
            gl::EnableVertexAttribArray(aNormal);
            gl::BindVertexArray();

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();

            return vao;
        }
    };

    template<typename TVert>
    static gl::Array_buffer alloc_sized_vbo(TVert const* verts, size_t n) {
        gl::Array_buffer rv;
        gl::BindBuffer(rv);
        gl::BufferData(rv.type, static_cast<GLsizeiptr>(sizeof(TVert) * n), verts, GL_STATIC_DRAW);
        return rv;
    }
}

namespace osmv {
    // mesh, fully loaded onto the GPU with whichever VAOs it needs initialized also
    struct Mesh_on_gpu final {
        gl::Array_buffer vbo;
        size_t nverts;
        gl::Array_bufferT<Mesh_instance> instance_vbo{static_cast<GLenum>(GL_DYNAMIC_DRAW)};
        gl::Vertex_array main_vao;
        gl::Vertex_array normal_vao;

    public:
        Mesh_on_gpu(osmv::Untextured_vert const* verts, size_t n) :
            vbo{alloc_sized_vbo(verts, n)},
            nverts{n},
            main_vao{Gouraud_mrt_shader::create_vao<gl::Array_buffer, Untextured_vert>(vbo, instance_vbo)},
            normal_vao{Normals_shader::create_vao<gl::Array_buffer, Untextured_vert>(vbo)} {

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
        }

        Mesh_on_gpu(osmv::Textured_vert const* verts, size_t n) :
            vbo{alloc_sized_vbo(verts, n)},
            nverts{n},
            main_vao{Gouraud_mrt_shader::create_vao<gl::Array_buffer, osmv::Textured_vert>(vbo, instance_vbo)},
            normal_vao{Normals_shader::create_vao<gl::Array_buffer, osmv::Textured_vert>(vbo)} {

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
        }

        int sizei() const noexcept {
            return static_cast<int>(nverts);
        }
    };
}

namespace {
    // create an OpenGL Pixel Buffer Object (PBO) that holds exactly one pixel
    gl::Pixel_pack_buffer make_single_pixel_PBO() {
        gl::Pixel_pack_buffer rv;
        gl::BindBuffer(rv);
        GLubyte rgb[4]{};  // initialize PBO's content to zeroed values
        gl::BufferData(rv.type, 4, rgb, GL_STREAM_READ);
        gl::UnbindBuffer(rv);
        return rv;
    }

    // OpenGL buffers used by the renderer
    //
    // designed with move + assignment semantics in-mind, so that users can just
    // reassign new Render_buffers over these ones (e.g. if drawing dimensions
    // change)
    struct Renderer_buffers final {};

    template<typename Vert>
    osmv::Mesh_reference allocate(std::vector<osmv::Mesh_on_gpu>& meshes, Vert const* verts, size_t n) {
        osmv::Mesh_reference ref = osmv::Mesh_reference::from_index(meshes.size());
        meshes.emplace_back(verts, n);
        return ref;
    }
}

struct osmv::Mesh_storage::Impl final {
    std::vector<Mesh_on_gpu> meshes;
};

osmv::Mesh_storage::Mesh_storage() : impl{new Mesh_storage::Impl{}} {
}

osmv::Mesh_storage::~Mesh_storage() noexcept {
    delete impl;
}

osmv::Mesh_on_gpu& osmv::Mesh_storage::lookup(Mesh_reference ref) const {
    size_t idx = ref.to_index();
    assert(idx < impl->meshes.size());
    return impl->meshes[idx];
}

osmv::Mesh_reference osmv::Mesh_storage::allocate(Untextured_vert const* verts, size_t n) {
    return ::allocate(impl->meshes, verts, n);
}

osmv::Mesh_reference osmv::Mesh_storage::allocate(Textured_vert const* verts, size_t n) {
    return ::allocate(impl->meshes, verts, n);
}

struct osmv::Texture_storage::Impl final {
    std::vector<gl::Texture_2d> textures;
};

osmv::Texture_storage::Texture_storage() : impl{new Texture_storage::Impl{}} {
}

osmv::Texture_storage::~Texture_storage() noexcept {
    delete impl;
}

gl::Texture_2d& osmv::Texture_storage::lookup(Texture_reference ref) const {
    size_t idx = ref.to_index();
    assert(idx < impl->textures.size());
    return impl->textures[idx];
}

osmv::Texture_reference osmv::Texture_storage::allocate(gl::Texture_2d&& tex) {
    osmv::Texture_reference ref = osmv::Texture_reference::from_index(impl->textures.size());
    impl->textures.emplace_back(std::move(tex));
    return ref;
}

struct osmv::Shader_cache::Impl final {
    Gouraud_mrt_shader gouraud;
    Normals_shader normals;
    Plain_texture_shader plain_texture;
    Colormapped_plain_texture_shader colormapped_plain_texture;
    Edge_detection_shader edge_detection_shader;
    Skip_msxaa_blitter_shader skip_msxaa_shader;

    Impl() {
        OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
    }
};

osmv::Shader_cache::Shader_cache() : impl{new Shader_cache::Impl{}} {
}

osmv::Shader_cache::~Shader_cache() noexcept {
    delete impl;
}

Gouraud_mrt_shader& osmv::Shader_cache::gouraud() const {
    return impl->gouraud;
}

Normals_shader& osmv::Shader_cache::normals() const {
    return impl->normals;
}

Plain_texture_shader& osmv::Shader_cache::pts() const {
    return impl->plain_texture;
}

Colormapped_plain_texture_shader& osmv::Shader_cache::colormapped_pts() const {
    return impl->colormapped_plain_texture;
}

Edge_detection_shader& osmv::Shader_cache::edge_detector() const {
    return impl->edge_detection_shader;
}

Skip_msxaa_blitter_shader& osmv::Shader_cache::skip_msxaa() const {
    return impl->skip_msxaa_shader;
}

struct osmv::Render_target::Impl final {
    // dimensions that these buffers were initialized with
    int w;
    int h;

    // num multisamples that these buffers were initialized with
    int samples;

    // buffers for main scene render (MSXAAed, MRT output, etc.)
    struct Scene {
        // stores multisampled scene render /w shading
        gl::Render_buffer color0;

        // stores COLOR1 RGB passthrough (selection logic)
        //
        // this is a texture because color picking (hover) logic needs to access exactly
        // one sample in it with a specialized shader
        gl::Texture_2d_multisample color1;

        // stores depth + stencil buffer for main FBO
        gl::Render_buffer depth24stencil8;

        // fbo for the above storage
        gl::Frame_buffer fbo;

        Scene(int w, int h, int samps) :
            // allocate COLOR0
            color0{[w, h, samps]() {
                gl::Render_buffer rv;
                gl::BindRenderBuffer(rv);
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, samps, GL_RGBA, w, h);
                return rv;
            }()},

            // allocate COLOR1
            color1{[w, h, samps]() {
                gl::Texture_2d_multisample rv;
                gl::BindTexture(rv);
                glTexImage2DMultisample(rv.type, samps, GL_RGB, w, h, GL_TRUE);
                return rv;
            }()},

            // allocate depth + stencil RBO
            depth24stencil8{[w, h, samps]() {
                gl::Render_buffer rv;
                gl::BindRenderBuffer(rv);
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, samps, GL_DEPTH24_STENCIL8, w, h);
                return rv;
            }()},

            // allocate FBO that links all of the above
            fbo{[this]() {
                gl::Frame_buffer rv;

                // configure main FBO
                gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color0);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, color1.type, color1, 0);
                gl::FramebufferRenderbuffer(
                    GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth24stencil8);

                // check it's OK
                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);

                return rv;
            }()} {

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
        }
    } scene;

    // non-MSXAAed FBO for sampling raw color values
    //
    // used to sample raw passthrough RGB to decode selection logic
    struct Non_msxaaed final {

        // output storage
        gl::Texture_2d tex;

        // fbo that links to the storage
        gl::Frame_buffer fbo;

        Non_msxaaed(int w, int h) :
            // allocate output storage
            tex{[w, h]() {
                gl::Texture_2d rv;

                // allocate non-MSXAA texture for non-blended sampling
                gl::BindTexture(rv);
                gl::TexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

                return rv;
            }()},

            // attach COLOR0 to output storage
            fbo{[this]() {
                gl::Frame_buffer rv;

                // configure non-MSXAA fbo
                gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.type, tex, 0);

                // check non-MSXAA OK
                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);

                return rv;
            }()} {

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
        }
    } skip_msxaa;

    // basic non-MSXAAed pairing of a 2d texture with an FBO for writing to
    // the texture
    struct Basic_fbo_texture_pair final {
        gl::Texture_2d tex;
        gl::Frame_buffer fbo;

        Basic_fbo_texture_pair(int w, int h, GLenum format) :
            tex{[w, h, format]() {
                gl::Texture_2d rv;
                gl::BindTexture(rv);
                gl::TexImage2D(rv.type, 0, static_cast<int>(format), w, h, 0, format, GL_UNSIGNED_BYTE, nullptr);
                gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
                gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
                return rv;
            }()},
            fbo{[this]() {
                gl::Frame_buffer rv;
                gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.type, tex, 0);

                assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);

                return rv;
            }()} {

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
        }
    };

    Basic_fbo_texture_pair color0_resolved;

    // target for resolved (post-MSXAA) COLOR1 RGB passthrough (selection logic)
    //
    // this isn't strictly necessary, but is useful to have so that we can render RGB2 to
    // a debug quad
    Basic_fbo_texture_pair color1_resolved;

    // pixel buffer objects (PBOs) for storing pixel color values
    //
    // these are used to asychronously request the pixel under the user's mouse
    // such that the renderer can decode that pixel value *on the next frame*
    // without stalling the GPU pipeline
    std::array<gl::Pixel_pack_buffer, 2> pbos{make_single_pixel_PBO(), make_single_pixel_PBO()};
    int pbo_idx = 0;  // 0 or 1

    // TODO: the renderer may not necessarily be drawing into the application screen
    //       and may, instead, be drawing into an arbitrary FBO (e.g. for a panel, or
    //       video recording), so the renderer shouldn't assume much about the app
    Impl(int _w, int _h, int _samples) :
        w{_w},
        h{_h},
        samples{_samples},
        scene{w, h, samples},
        skip_msxaa{w, h},
        color0_resolved{w, h, GL_RGBA},
        color1_resolved{w, h, GL_RGB} {

        OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
    }
};

osmv::Render_target::Render_target(int w, int h, int samples) : impl{new Render_target::Impl{w, h, samples}} {
}

osmv::Render_target::~Render_target() noexcept {
    delete impl;
}

osmv::Render_target::Impl& osmv::Render_target::raw_impl() const noexcept {
    return *impl;
}

void osmv::Render_target::reconfigure(int w, int h, int samples) {
    Render_target::Impl& cur = *impl;
    if (w != cur.w or h != cur.h or samples != cur.samples) {
        delete impl;
        impl = new Render_target::Impl{w, h, samples};
    }
}

glm::vec2 osmv::Render_target::dimensions() const noexcept {
    return {static_cast<float>(impl->w), static_cast<float>(impl->h)};
}

int osmv::Render_target::samples() const noexcept {
    return impl->samples;
}

float osmv::Render_target::aspect_ratio() const noexcept {
    auto dims = dimensions();
    return dims.x / dims.y;
}

gl::Texture_2d& osmv::Render_target::main() noexcept {
    return impl->color0_resolved.tex;
}

struct osmv::Renderer::Impl final {
    // debug quad
    gl::Array_bufferT<osmv::Textured_vert> quad_vbo = osmv::shaded_textured_quad_verts();
    gl::Vertex_array edge_detection_quad_vao = Edge_detection_shader::create_vao(quad_vbo);
    gl::Vertex_array skip_msxaa_quad_vao = Skip_msxaa_blitter_shader::create_vao(quad_vbo);
    gl::Vertex_array pts_quad_vao = Plain_texture_shader::create_vao(quad_vbo);
    gl::Vertex_array cpts_quad_vao = Colormapped_plain_texture_shader::create_vao(quad_vbo);
};

// ok, this took an inordinate amount of time, but there's a fucking
// annoying bug in Clang:
//
// https://bugs.llvm.org/show_bug.cgi?id=28280
//
// where you'll get insane link errors if you use curly bracers to init
// the state. I *think* it's because there's a mix of noexcept and except
// dtors (Simbody uses exceptional dtors...)
//
// DO NOT USE CURLY BRACERS HERE
osmv::Renderer::Renderer() : impl(new Renderer::Impl()) {
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
}

osmv::Renderer::~Renderer() noexcept {
    delete impl;
}

osmv::Passthrough_data osmv::Renderer::draw(
    Gpu_storage const& storage, Raw_drawcall_params const& params, Drawlist const& drawlist, Render_target& out) {

    // overview:
    //
    // drawing the scene efficiently is a fairly involved process. I apologize for that, but
    // rendering scenes efficiently with OpenGL requires has to keep OpenGL, GPUs, and API
    // customization in-mind - while also playing ball with the OpenSim API.
    //
    // this is a forward (as opposed to deferred) renderer that borrows some ideas from deferred
    // rendering techniques. It *mostly* draws the entire scene in one pass (forward rending) but
    // the rendering step *also* writes to a multi-render-target (MRT) FBO that extra information
    // such as what's currently selected, and it uses that information in downstream sampling steps
    // (kind of like how deferred rendering puts everything into information-dense buffers). The
    // reason this rendering pipeline isn't fully deferred (gbuffers, albeido, etc.) is because
    // the scene is lit by a single directional light and the shading is fairly simple.

    Mesh_instance const* meshes = drawlist.instances.data();
    size_t nmeshes = drawlist.instances.size();

#ifndef NDEBUG
    // debug OpenGL: ensure there are no OpenGL errors before setup
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

    Render_target::Impl& buffers = out.raw_impl();

    glViewport(0, 0, buffers.w, buffers.h);

    // bind to an off-screen framebuffer object (FBO)
    //
    // drawing into this FBO writes to textures that the user can't see, but that can
    // be sampled by downstream shaders
    gl::BindFrameBuffer(GL_FRAMEBUFFER, buffers.scene.fbo);

    // clear the scene FBO's draw buffers for a new draw call
    //
    //   - COLOR0: main scene render: fill in background
    //   - COLOR1: RGB passthrough (selection logic + rim alpa): blank out all channels
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::ClearColor(params.background_rgba);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::DrawBuffer(GL_COLOR_ATTACHMENT1);
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT);

    // handle wireframe mode: should only be enabled for scene + floor render: the other
    // renders will render to a screen-sized quad
    GLenum original_poly_mode = gl::GetEnum(GL_POLYGON_MODE);
    if (params.flags & DrawcallFlags_WireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

#ifndef NDEBUG
    // debug OpenGL: ensure no OpenGL errors after initial buffer clears, draw setup
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

    // render the scene to the FBO using a multiple-render-target (MRT) multisampled
    // (MSXAAed) shader.
    //
    // FBO outputs are:
    //
    // - COLOR0: main target: multisampled scene geometry
    //     - the input color is Gouraud-shaded based on light parameters etc.
    // - COLOR1: RGB passthrough: written to output as-is
    //     - the input color encodes the selected component index (RG) and the rim
    //       alpha (B). It's used in downstream steps
    if (params.flags & RawRendererFlags_DrawSceneGeometry) {
        Gouraud_mrt_shader& shader = storage.shaders.gouraud();

        gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, params.projection_matrix);
        gl::Uniform(shader.uViewMat, params.view_matrix);
        gl::Uniform(shader.uLightPos, params.light_pos);
        gl::Uniform(shader.uLightColor, params.light_rgb);
        gl::Uniform(shader.uViewPos, params.view_pos);

        // blending:
        //     COLOR0 should be blended because OpenSim scenes can contain blending
        //     COLOR1 should never be blended: it's a value for the top-most fragment
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnablei(GL_BLEND, 0);
        glDisablei(GL_BLEND, 1);

        if (params.flags & RawRendererFlags_UseInstancedRenderer) {
            size_t pos = 0;
            while (pos < nmeshes) {
                Mesh_reference meshid = meshes[pos]._meshid;
                Texture_reference textureid = meshes[pos]._diffuse_texture;
                size_t end = pos + 1;

                while (end < nmeshes and meshes[end]._meshid == meshid and meshes[end]._diffuse_texture == textureid) {
                    ++end;
                }

                // [pos, end) contains instances with the same meshid + textureid

                if (textureid) {
                    gl::Uniform(shader.uIsTextured, true);
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(storage.textures.lookup(textureid));
                    gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
                } else {
                    gl::Uniform(shader.uIsTextured, false);
                }

                Mesh_on_gpu& md = storage.meshes.lookup(meshid);
                md.instance_vbo.assign(meshes + pos, meshes + end);
                gl::BindVertexArray(md.main_vao);
                glDrawArraysInstanced(GL_TRIANGLES, 0, md.sizei(), static_cast<GLsizei>(end - pos));
                gl::BindVertexArray();

                pos = end;
            }
        } else {
            // perform (slower) one-drawcall-per-item rendering
            //
            // this is here mostly for perf comparison and debugging

            for (size_t i = 0; i < nmeshes; ++i) {
                Mesh_instance const& mi = meshes[i];
                if (mi._diffuse_texture) {
                    gl::Uniform(shader.uIsTextured, true);
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(storage.textures.lookup(mi._diffuse_texture));
                    gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
                } else {
                    gl::Uniform(shader.uIsTextured, false);
                }
                Mesh_on_gpu& md = storage.meshes.lookup(mi._meshid);
                md.instance_vbo.assign(meshes + i, meshes + i + 1);
                gl::BindVertexArray(md.main_vao);
                glDrawArraysInstanced(GL_TRIANGLES, 0, md.sizei(), static_cast<GLsizei>(1));
                gl::BindVertexArray();
            }
        }

        glDisablei(GL_BLEND, 0);
    }

    glPolygonMode(GL_FRONT_AND_BACK, original_poly_mode);

    // (optional): render scene normals into COLOR0
    if (params.flags & DrawcallFlags_ShowMeshNormals) {
        Normals_shader& shader = storage.shaders.normals();
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, params.projection_matrix);
        gl::Uniform(shader.uViewMat, params.view_matrix);

        for (size_t i = 0; i < nmeshes; ++i) {
            Mesh_instance const& mi = meshes[i];
            Mesh_on_gpu& md = storage.meshes.lookup(mi._meshid);
            gl::Uniform(shader.uModelMat, mi.transform);
            gl::Uniform(shader.uNormalMat, mi._normal_xform);
            gl::BindVertexArray(md.normal_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, md.sizei());
        }
        gl::BindVertexArray();
    }

#ifndef NDEBUG
    // debug OpenGL: ensure there are no errors after drawing the scene
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

    // perform passthrough hit testing
    //
    // in the previous draw call, COLOR1's RGB channels encoded arbitrary passthrough data
    // Extracting that pixel value (without MSXAA blending) and decoding it yields the
    // user-supplied data
    //
    // this makes it possible for renderer users (e.g. OpenSim model renderer) to encode
    // model information (e.g. "a component index") into screenspace

    Passthrough_data hittest_result{};
    if (params.flags & RawRendererFlags_PerformPassthroughHitTest) {
        // (temporarily) set the OpenGL viewport to a small square around the hit testing
        // location
        //
        // this causes the subsequent draw call to only run the fragment shader around where
        // we actually care about
        glViewport(params.passthrough_hittest_x - 1, params.passthrough_hittest_y - 1, 3, 3);

        // bind to a non-MSXAAed FBO
        gl::BindFrameBuffer(GL_FRAMEBUFFER, buffers.skip_msxaa.fbo);
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

        // use a specialized shader that is MSXAA-aware to blit exactly one non-blended AA
        // sample from COLOR1 to the output
        //
        // by deliberately avoiding MSXAA, every value in this output should be exactly the
        // same as the passthrough value provided by the caller
        Skip_msxaa_blitter_shader& shader = storage.shaders.skip_msxaa();
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(buffers.scene.color1);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(impl->skip_msxaa_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, impl->quad_vbo.sizei());
        gl::BindVertexArray();

        // reset viewport
        glViewport(0, 0, buffers.w, buffers.h);

        // the FBO now contains non-MSXAAed version of COLOR1

        // read the pixel under the mouse
        //
        // - you *could* just read the value directly from the FBO with `glReadPixels`, which is
        //   what the first iteration of this alg. did (non optimized)
        //
        // - However, that glReadPixels call will screw performance. On my machine (Ryzen1600 /w
        //   Geforce 1060), it costs around 30 % FPS (300 FPS --> 200 FPS)
        //
        // - This isn't because the transfer is expensive--it's just a single pixel, after all--but
        //   because reading the pixel forces the OpenGL driver to flush all pending rendering
        //   operations to the FBO (a "pipeline stall")
        //
        // - So this algorithm uses a crafty trick, which is to use two pixel buffer objects (PBOs)
        //   to asynchronously transfer the pixel *from the previous frame* into CPU memory using
        //   asynchronous DMA. The trick uses two PBOs, which each of which are either:
        //
        //   1. Requesting the pixel value (via glReadPixel). The OpenGL spec does *not* require
        //      that the PBO is populated once `glReadPixel` returns, so this does not cause a
        //      pipeline stall
        //
        //   2. Mapping the PBO that requested a pixel value **on the last frame**. The OpenGL spec
        //      requires that this PBO is populated once the mapping is enabled, so this will
        //      stall the pipeline. However, that pipeline stall will be on the *previous* frame
        //      which is less costly to stall on

        if (params.flags & RawRendererFlags_UseOptimizedButDelayed1FrameHitTest) {
            int reader = buffers.pbo_idx % static_cast<int>(buffers.pbos.size());
            int mapper = (buffers.pbo_idx + 1) % static_cast<int>(buffers.pbos.size());

            // launch asynchronous request for this frame's pixel
            gl::BindBuffer(buffers.pbos[static_cast<size_t>(reader)]);
            glReadPixels(
                params.passthrough_hittest_x, params.passthrough_hittest_y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

            // synchrnously read *last frame's* pixel
            gl::BindBuffer(buffers.pbos[static_cast<size_t>(mapper)]);
            GLubyte* src = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));

            // note: these values are the *last frame*'s
            hittest_result.b0 = src[0];
            hittest_result.b1 = src[1];

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

            // flip PBOs ready for next frame
            buffers.pbo_idx = (buffers.pbo_idx + 1) % static_cast<int>(buffers.pbos.size());
        } else {
            // slow mode: synchronously read the current frame's pixel under the cursor
            //
            // this is kept here so that people can try it out if selection logic is acting
            // bizzarely (e.g. because it is delayed one frame)

            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            GLubyte rgba[3]{};
            glReadPixels(
                params.passthrough_hittest_x, params.passthrough_hittest_y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgba);

            hittest_result.b0 = rgba[0];
            hittest_result.b1 = rgba[1];
        }
    }

#ifndef NDEBUG
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

    // resolve MSXAA in COLOR0 to output texture
    //
    // "resolve" (i.e. blend) the MSXAA samples in COLOR0. We are "done" with
    // COLOR0. You might expect we can directly blit it to the output, but that
    // seems to explode with some OpenGL drivers (e.g. Intel iGPUs like UHD 620)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, buffers.scene.fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffers.color0_resolved.fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, buffers.w, buffers.h, 0, 0, buffers.w, buffers.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

#ifndef NDEBUG
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

    // resolve MSXAA in COLOR1
    //
    // "resolve" (i.e. blend) the MSXAA samples in COLOR1 into non-MSXAAed textures
    // that the edge-detection shader can sample normally
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, buffers.scene.fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffers.color1_resolved.fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, buffers.w, buffers.h, 0, 0, buffers.w, buffers.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

#ifndef NDEBUG
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

    // bind to output texture: all further drawing goes onto it
    gl::BindFrameBuffer(GL_FRAMEBUFFER, buffers.color0_resolved.fbo);

#ifndef NDEBUG
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

    // draw rims highlights onto the output
    //
    // COLOR1's alpha channel contains *filled in shapes* for each element in the scene that
    // should be rim-shaded. Those shapes are exactly the same as the scene geometry, so showing
    // them as-is would be pointless (they'd either entirely occlude, or be occluded by, the scene)
    //
    // rim-highlighting puts a rim around the outer edge of the geometry. There are various tricks
    // for doing this, such as rendering the geometry twice - the second time backface-enlarged
    // slightly, or holding onto two versions of every mesh (normal mesh, normal-scaled mesh),
    // but those techniques each have drawbacks (e.g. more draw calls, fails with non-convex
    // geometry, behaves bizzarely with non-centered meshes)
    //
    // this technique performs rim highlighting in screen-space using a standard edge-detection
    // kernel. The drawback of this is that every single pixel in the screen has to be
    // edge-detected, and the rims are in screen-space, rather than world space (so they don't
    // "zoom out" as if they were "in the scene"). However, GPUs are fairly efficient at running
    // branchless kernel lookups over a screen, so it isn't as expensive as you think
    if (params.flags & DrawcallFlags_DrawRims) {
        Edge_detection_shader& shader = storage.shaders.edge_detector();
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(buffers.color1_resolved.tex);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::Uniform(shader.uRimRgba, params.rim_rgba);

        float rim_thickness = 2.0f / std::max(buffers.w, buffers.h);

        gl::Uniform(shader.uRimThickness, rim_thickness);

        glEnable(GL_BLEND);  // rims can have alpha
        glDisable(GL_DEPTH_TEST);
        gl::BindVertexArray(impl->edge_detection_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, impl->quad_vbo.sizei());
        gl::BindVertexArray();
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

#ifndef NDEBUG
    // debug OpenGL: ensure no errors after drawing rim overlay
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

    // render debug quads onto output (if applicable)
    if (params.flags & RawRendererFlags_DrawDebugQuads) {
        Colormapped_plain_texture_shader& cpts = storage.shaders.colormapped_pts();
        gl::UseProgram(cpts.p);
        gl::BindVertexArray(impl->pts_quad_vao);

        // COLOR1 quad (RGB)
        if (true) {
            glm::mat4 row1 = []() {
                glm::mat4 m = glm::identity<glm::mat4>();
                m = glm::translate(m, glm::vec3{0.80f, 0.80f, -1.0f});  // move to [+0.6, +1.0] in x
                m = glm::scale(m, glm::vec3{0.20f});  // so it becomes [-0.2, +0.2]
                return m;
            }();

            gl::Uniform(cpts.uMVP, row1);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(buffers.color1_resolved.tex);
            gl::Uniform(cpts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(cpts.uSamplerMultiplier, gl::identity_val);
            gl::DrawArrays(GL_TRIANGLES, 0, impl->quad_vbo.sizei());
        }

        // COLOR1 quad (A)
        if (true) {
            glm::mat4 row2 = []() {
                glm::mat4 m = glm::identity<glm::mat4>();
                m = glm::translate(m, glm::vec3{0.80f, 0.40f, -1.0f});  // move to [+0.6, +1.0] in x
                m = glm::scale(m, glm::vec3{0.20f});  // so it becomes [-0.2, +0.2]
                return m;
            }();

            glm::mat4 alpha2rgb = {
                // column-major
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                1.0f,
            };

            gl::Uniform(cpts.uMVP, row2);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(buffers.color1_resolved.tex);
            gl::Uniform(cpts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(cpts.uSamplerMultiplier, alpha2rgb);
            gl::DrawArrays(GL_TRIANGLES, 0, impl->quad_vbo.sizei());
        }

        gl::BindVertexArray();
    }

#ifndef NDEBUG
    // debug OpenGL: ensure no errors after drawing debug quads
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

    // bind back to the original framebuffer (assumed to be window)
    gl::BindFrameBuffer(GL_FRAMEBUFFER, gl::window_fbo);

    return hittest_result;
}

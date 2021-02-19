#include "raw_renderer.hpp"

#include "3d_common.hpp"
#include "gl.hpp"
#include "src/config.hpp"

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <exception>
#include <limits>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
    static_assert(
        std::is_trivially_copyable<osmv::Mesh_instance>::value,
        "a mesh instance should ideally be trivially copyable, because it is potentially used *a lot* in draw calls. You can remove this assert if you disagree");
    static_assert(
        std::is_standard_layout<osmv::Mesh_instance>::value,
        "this is required for offsetof macro usage, which is used for setting up OpenGL attribute pointers. See: https://en.cppreference.com/w/cpp/types/is_standard_layout");
    static_assert(
        std::is_trivially_constructible<osmv::Mesh_instance>::value,
        "this is a nice-to-have, because it enables bulk-allocating mesh instances in a collection class with zero overhead");
    static_assert(
        std::is_trivially_destructible<osmv::Mesh_instance>::value,
        "this is a nice-to-have, because it enables bulk-destroying mesh instances in a collection class with zero overhead");

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

    void u8_to_Vec4Pointer(gl::Attribute const& vec4log, size_t base_offset) {
        glVertexAttribPointer(
            vec4log, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(osmv::Mesh_instance), reinterpret_cast<void*>(base_offset));
        glEnableVertexAttribArray(vec4log);
        glVertexAttribDivisor(vec4log, 1);
    }

    // An instanced multi-render-target (MRT) shader that performes Gouraud shading for
    // COLOR0 and RGBA passthrough for COLOR1
    //
    // - COLOR0: geometry colored with Gouraud shading: i.e. "the scene"
    // - COLOR1: RGBA passthrough (selection logic + rim alphas)
    struct Gouraud_mrt_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::config::shader_path("gouraud_mrt.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::config::shader_path("gouraud_mrt.frag")));

        // vertex attrs
        static constexpr gl::Attribute aLocation = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);

        // instancing attrs
        static constexpr gl::Attribute aModelMat = gl::AttributeAtLocation(2);
        static constexpr gl::Attribute aNormalMat = gl::AttributeAtLocation(6);
        static constexpr gl::Attribute aRgba0 = gl::AttributeAtLocation(9);
        static constexpr gl::Attribute aRgba1 = gl::AttributeAtLocation(10);

        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(program, "uProjMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(program, "uViewMat");
        gl::Uniform_vec3 uLightPos = gl::GetUniformLocation(program, "uLightPos");
        gl::Uniform_vec3 uLightColor = gl::GetUniformLocation(program, "uLightColor");
        gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(program, "uViewPos");

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

            gl::BindBuffer(instance_vbo);
            Mat4Pointer(aModelMat, offsetof(osmv::Mesh_instance, transform));
            Mat3Pointer(aNormalMat, offsetof(osmv::Mesh_instance, _normal_xform));
            u8_to_Vec4Pointer(aRgba0, offsetof(osmv::Mesh_instance, rgba));
            u8_to_Vec4Pointer(aRgba1, offsetof(osmv::Mesh_instance, _passthrough));

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

    // mesh, fully loaded onto the GPU with whichever VAOs it needs initialized also
    struct Mesh_on_gpu final {
        gl::Array_bufferT<osmv::Untextured_vert> vbo;
        size_t instance_hash = 0;  // cache VBO assignments
        gl::Array_bufferT<osmv::Mesh_instance> instance_vbo{static_cast<GLenum>(GL_DYNAMIC_DRAW)};
        gl::Vertex_array main_vao;
        gl::Vertex_array normal_vao;

    public:
        Mesh_on_gpu(osmv::Untextured_vert const* verts, size_t n) :
            vbo{verts, verts + n},
            main_vao{Gouraud_mrt_shader::create_vao(vbo, instance_vbo)},
            normal_vao{Normals_shader::create_vao(vbo)} {

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
        }

        int sizei() const noexcept {
            return vbo.sizei();
        }
    };

    // create an OpenGL Pixel Buffer Object (PBO) that holds exactly one pixel
    gl::Pixel_pack_buffer make_single_pixel_PBO() {
        gl::Pixel_pack_buffer rv;
        gl::BindBuffer(rv);
        GLubyte rgba[4]{};  // initialize PBO's content to zeroed values
        gl::BufferData(rv.type, 4, rgba, GL_STREAM_READ);
        gl::UnbindBuffer(rv);
        return rv;
    }

    // this global exists because it makes handling mesh allocations between
    // different parts of the application *much* simpler. We "know" that meshids
    // are globally unique, and that there is one global API for allocating them
    // (OpenGL). It also means that the rest of the appliation can use trivial
    // types (ints) which is handy when they are composed with other trivial
    // types into large buffers that need to be memcopied around (e.g. mesh
    // instance data)
    //
    // this should only be populated after OpenGL is initialized
    std::vector<Mesh_on_gpu> global_meshes;

    Mesh_on_gpu& global_mesh_lookup(int meshid) {
        std::vector<Mesh_on_gpu>& meshes = global_meshes;

        assert(meshid != osmv::invalid_meshid);
        assert(meshid >= 0);
        assert(static_cast<size_t>(meshid) < meshes.size());

        return meshes[static_cast<size_t>(meshid)];
    }

    // OpenGL buffers used by the renderer
    //
    // designed with move + assignment semantics in-mind, so that users can just
    // reassign new Render_buffers over these ones (e.g. if drawing dimensions
    // change)
    struct Renderer_buffers final {

        // dimensions that these buffers were initialized with
        int w;
        int h;

        // num multisamples that these buffers were initialized with
        int samples;

        // buffers for main scene render (MSXAAed, MRT output, etc.)
        struct Scene {
            // stores multisampled scene render /w shading
            gl::Render_buffer color0;

            // stores COLOR1 RGBA passthrough (selection logic)
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
                    glTexImage2DMultisample(rv.type, samps, GL_RGBA, w, h, GL_TRUE);
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
        // used to sample raw passthrough RGBA to decode selection logic
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
                    gl::TexImage2D(rv.type, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

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

        // target for resolved (post-MSXAA) COLOR1 RGBA passthrough (selection logic)
        //
        // this isn't strictly necessary, but is useful to have so that we can render RGBA2 to
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
        Renderer_buffers(int _w, int _h, int _samples) :
            w{_w},
            h{_h},
            samples{_samples},
            scene{w, h, samples},
            skip_msxaa{w, h},
            color0_resolved{w, h, GL_RGBA},
            color1_resolved{w, h, GL_RGBA} {

            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
        }
    };
}

namespace osmv {
    // internal renderer implementation details
    struct Renderer_impl final {

        struct Shaders {
            Gouraud_mrt_shader gouraud;
            Normals_shader normals;
            Plain_texture_shader plain_texture;
            Colormapped_plain_texture_shader colormapped_plain_texture;
            Edge_detection_shader edge_detection_shader;
            Skip_msxaa_blitter_shader skip_msxaa_shader;

            Shaders() {
                OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
            }
        } shaders;

        // debug quad
        gl::Array_bufferT<osmv::Shaded_textured_vert> quad_vbo = osmv::shaded_textured_quad_verts;
        gl::Vertex_array edge_detection_quad_vao = Edge_detection_shader::create_vao(quad_vbo);
        gl::Vertex_array skip_msxaa_quad_vao = Skip_msxaa_blitter_shader::create_vao(quad_vbo);
        gl::Vertex_array pts_quad_vao = Plain_texture_shader::create_vao(quad_vbo);
        gl::Vertex_array cpts_quad_vao = Colormapped_plain_texture_shader::create_vao(quad_vbo);

        // floor
        struct {
            gl::Texture_2d texture = osmv::generate_chequered_floor_texture();
            glm::mat4 model_mtx = []() {
                glm::mat4 rv = glm::identity<glm::mat4>();

                // OpenSim: might contain floors at *exactly* Y = 0.0, so shift the chequered
                // floor down *slightly* to prevent Z fighting from planes rendered from the
                // model itself (the contact planes, etc.)
                rv = glm::translate(rv, {0.0f, -0.001f, 0.0f});

                rv = glm::rotate(rv, osmv::pi_f / 2, {-1.0, 0.0, 0.0});
                rv = glm::scale(rv, {100.0f, 100.0f, 0.0f});

                return rv;
            }();
        } floor;

        // other OpenGL (GPU) buffers used by the renderer
        Renderer_buffers buffers;

        Renderer_impl(Raw_renderer_config const& settings) : buffers{settings.w, settings.h, settings.samples} {
            OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
        }
    };
}

int osmv::globally_allocate_mesh(osmv::Untextured_vert const* verts, size_t n) {
    assert(global_meshes.size() < std::numeric_limits<int>::max());
    int meshid = static_cast<int>(global_meshes.size());
    global_meshes.emplace_back(verts, n);
    return meshid;
}

void osmv::optimize_draw_order(Mesh_instance* mi, size_t n) noexcept {
    std::sort(mi, mi + n, [](osmv::Mesh_instance const& a, osmv::Mesh_instance const& b) {
        return a.rgba.a != b.rgba.a ? a.rgba.a > a.rgba.b : a._meshid < b._meshid;
    });
}

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
osmv::Raw_renderer::Raw_renderer(osmv::Raw_renderer_config const& _settings) : impl(new Renderer_impl(_settings)) {
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
}

osmv::Raw_renderer::~Raw_renderer() noexcept {
    delete impl;
}

void osmv::Raw_renderer::change_config(Raw_renderer_config const& cfg) {
    Renderer_buffers& b = impl->buffers;
    if (cfg.w != b.w or cfg.h != b.h or cfg.samples != b.samples) {
        impl->buffers = Renderer_buffers{cfg.w, cfg.h, cfg.samples};
    }
}

glm::vec2 osmv::Raw_renderer::dimensions() const noexcept {
    return {static_cast<float>(impl->buffers.w), static_cast<float>(impl->buffers.h)};
}

float osmv::Raw_renderer::aspect_ratio() const noexcept {
    glm::vec2 d = dimensions();
    return d.x / d.y;
}

osmv::Raw_drawcall_result
    osmv::Raw_renderer::draw(Raw_drawcall_params const& params, Mesh_instance const* meshes, size_t nmeshes) {
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

#ifndef NDEBUG
    // debug OpenGL: ensure there are no OpenGL errors before setup
    OSMV_ASSERT_NO_OPENGL_ERRORS_HERE();
#endif

    Renderer_buffers& buffers = impl->buffers;

    glViewport(0, 0, buffers.w, buffers.h);

    // bind to an off-screen framebuffer object (FBO)
    //
    // drawing into this FBO writes to textures that the user can't see, but that can
    // be sampled by downstream shaders
    gl::BindFrameBuffer(GL_FRAMEBUFFER, buffers.scene.fbo);

    // clear the scene FBO's draw buffers for a new draw call
    //
    //   - COLOR0: main scene render: fill in background
    //   - COLOR1: RGBA passthrough (selection logic + rim alpa): blank out all channels
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::ClearColor(params.background_rgba);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::DrawBuffer(GL_COLOR_ATTACHMENT1);
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT);

    // handle wireframe mode: should only be enabled for scene + floor render: the other
    // renders will render to a screen-sized quad
    GLenum original_poly_mode = gl::GetEnum(GL_POLYGON_MODE);
    if (params.flags & RawRendererFlags_WireframeMode) {
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
    // - COLOR1: RGBA passthrough: written to output as-is
    //     - the input color encodes the selected component index (RGB) and the rim
    //       alpha (A). It's used in downstream steps
    if (params.flags & RawRendererFlags_DrawSceneGeometry) {
        Gouraud_mrt_shader& shader = impl->shaders.gouraud;

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
        glDisablei(GL_BLEND, 1);
        glEnablei(GL_BLEND, 0);

        // instanced draw ordering
        //
        // cluster draw calls by meshid
        size_t pos = 0;
        while (pos < nmeshes) {
            int meshid = meshes[pos]._meshid;
            size_t end = pos + 1;

            while (end < nmeshes and meshes[end]._meshid == meshid) {
                ++end;
            }

            // [pos, end) contains instances with meshid
            Mesh_on_gpu& md = global_mesh_lookup(static_cast<int>(meshid));
            md.instance_vbo.assign(meshes + pos, meshes + end);
            gl::BindVertexArray(md.main_vao);
            glDrawArraysInstanced(GL_TRIANGLES, 0, md.sizei(), static_cast<GLsizei>(end - pos));

            pos = end;
        }
        gl::BindVertexArray();
        glDisablei(GL_BLEND, 0);
    }

    // (optional): draw a textured floor into COLOR0
    if (params.flags & RawRendererFlags_ShowFloor) {
        Plain_texture_shader& pts = impl->shaders.plain_texture;

        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::UseProgram(pts.p);
        gl::Uniform(pts.uMVP, params.projection_matrix * params.view_matrix * impl->floor.model_mtx);
        gl::Uniform(pts.uTextureScaler, 200.0f);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(impl->floor.texture);
        gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());

        gl::BindVertexArray(impl->pts_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, impl->quad_vbo.sizei());
        gl::BindVertexArray();
    }

    glPolygonMode(GL_FRONT_AND_BACK, original_poly_mode);

    // (optional): render scene normals into COLOR0
    if (params.flags & RawRendererFlags_ShowMeshNormals) {
        Normals_shader& shader = impl->shaders.normals;
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, params.projection_matrix);
        gl::Uniform(shader.uViewMat, params.view_matrix);

        for (size_t i = 0; i < nmeshes; ++i) {
            Mesh_instance const& mi = meshes[i];
            Mesh_on_gpu& md = global_mesh_lookup(mi._meshid);
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
        Skip_msxaa_blitter_shader& shader = impl->shaders.skip_msxaa_shader;
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
                params.passthrough_hittest_x, params.passthrough_hittest_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

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

            GLubyte rgba[4]{};
            glReadPixels(
                params.passthrough_hittest_x, params.passthrough_hittest_y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

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
    if (params.flags & RawRendererFlags_DrawRims) {
        Edge_detection_shader& shader = impl->shaders.edge_detection_shader;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(buffers.color1_resolved.tex);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::Uniform(shader.uRimRgba, params.rim_rgba);
        gl::Uniform(shader.uRimThickness, params.rim_thickness);

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
        Colormapped_plain_texture_shader& cpts = impl->shaders.colormapped_plain_texture;
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

    return Raw_drawcall_result{buffers.color0_resolved.tex, hittest_result};
}

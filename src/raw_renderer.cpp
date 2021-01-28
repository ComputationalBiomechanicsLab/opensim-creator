#include "raw_renderer.hpp"

#include "3d_common.hpp"
#include "cfg.hpp"
#include "gl.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace {
    static_assert(
        std::is_trivially_copyable<osmv::Mesh_instance>::value == true,
        "a mesh instance should ideally be trivially copyable, because it is potentially used *a lot* in draw calls. You can remove this assert if you disagree");

    // A multi-render-target (MRT) shader that performed Gouraud shading for
    // COLOR0 and RGBA passthrough for COLOR1
    //
    // - COLOR0: geometry colored with Gouraud shading: i.e. "the scene"
    // - COLOR1: RGBA passthrough (selection logic + rim alphas)
    struct Gouraud_mrt_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("gouraud_mrt.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("gouraud_mrt.frag")));

        // vertex attrs
        static constexpr gl::Attribute aLocation = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aNormal = gl::AttributeAtLocation(1);

        // instancing attrs
        static constexpr gl::Attribute aModelMat = gl::AttributeAtLocation(2);
        static constexpr gl::Attribute aNormalMat = gl::AttributeAtLocation(6);
        static constexpr gl::Attribute aRgba0 = gl::AttributeAtLocation(10);
        static constexpr gl::Attribute aRgba1 = gl::AttributeAtLocation(11);

        gl::Uniform_mat4 uProjMat = gl::GetUniformLocation(program, "uProjMat");
        gl::Uniform_mat4 uViewMat = gl::GetUniformLocation(program, "uViewMat");
        gl::Uniform_vec3 uLightPos = gl::GetUniformLocation(program, "uLightPos");
        gl::Uniform_vec3 uLightColor = gl::GetUniformLocation(program, "uLightColor");
        gl::Uniform_vec3 uViewPos = gl::GetUniformLocation(program, "uViewPos");

        template<typename Vbo, typename T = typename Vbo::value_type>
        static gl::Vertex_array create_vao(Vbo& vbo) {
            gl::Vertex_array vao = gl::GenVertexArrays();

            gl::BindVertexArray(vao);
            gl::BindBuffer(vbo);
            gl::VertexAttribPointer(
                aLocation, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, pos)));
            gl::EnableVertexAttribArray(aLocation);
            gl::VertexAttribPointer(
                aNormal, 3, GL_FLOAT, GL_FALSE, sizeof(T), reinterpret_cast<void*>(offsetof(T, normal)));
            gl::EnableVertexAttribArray(aNormal);
            gl::BindVertexArray();

            return vao;
        }
    };

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

    void Vec4Pointer(gl::Attribute const& vec4log, size_t base_offset) {
        glVertexAttribPointer(
            vec4log, 4, GL_FLOAT, GL_FALSE, sizeof(osmv::Mesh_instance), reinterpret_cast<void*>(base_offset));
        glEnableVertexAttribArray(vec4log);
        glVertexAttribDivisor(vec4log, 1);
    }

    // A basic shader that just samples a texture onto the provided geometry
    //
    // useful for rendering quads etc.
    struct Plain_texture_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("plain_texture.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("plain_texture.frag")));

        static constexpr gl::Attribute aPos = gl::AttributeAtLocation(0);
        static constexpr gl::Attribute aTexCoord = gl::AttributeAtLocation(1);

        gl::Uniform_mat4 projMat = gl::GetUniformLocation(p, "projMat");
        gl::Uniform_mat4 viewMat = gl::GetUniformLocation(p, "viewMat");
        gl::Uniform_mat4 modelMat = gl::GetUniformLocation(p, "modelMat");
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

            return vao;
        }
    };

    // A specialized edge-detection shader for rim highlighting
    struct Edge_detection_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("edge_detect.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("edge_detect.frag")));

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

            return vao;
        }
    };

    struct Skip_msxaa_blitter_shader final {
        gl::Program p = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("skip_msxaa_blitter.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("skip_msxaa_blitter.frag")));

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

            return vao;
        }
    };

    // uses a geometry shader to render normals as lines
    struct Normals_shader final {
        gl::Program program = gl::CreateProgramFrom(
            gl::Compile<gl::Vertex_shader>(osmv::cfg::shader_path("draw_normals.vert")),
            gl::Compile<gl::Fragment_shader>(osmv::cfg::shader_path("draw_normals.frag")),
            gl::Compile<gl::Geometry_shader>(osmv::cfg::shader_path("draw_normals.geom")));

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
            return vao;
        }
    };

    // mesh, fully loaded onto the GPU with whichever VAOs it needs initialized also
    struct Mesh_on_gpu final {
        gl::Array_bufferT<osmv::Untextured_vert> vbo;
        gl::Vertex_array main_vao;
        gl::Vertex_array normal_vao;

    public:
        Mesh_on_gpu(osmv::Untextured_vert const* verts, size_t n) :
            vbo{verts, verts + n},
            main_vao{Gouraud_mrt_shader::create_vao(vbo)},
            normal_vao{Normals_shader::create_vao(vbo)} {
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

    gl::Array_bufferT<osmv::Mesh_instance>& get_mi_storage() {
        static gl::Array_bufferT<osmv::Mesh_instance> data;
        return data;
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

    // helper class: restore FBO to original value on destruction
    struct Restore_original_framebuffer_on_destruction final {
        GLint original_draw_fbo;
        GLint original_read_fbo;

        Restore_original_framebuffer_on_destruction() {
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &original_draw_fbo);
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &original_read_fbo);
        }
        Restore_original_framebuffer_on_destruction(Restore_original_framebuffer_on_destruction const&) = delete;
        Restore_original_framebuffer_on_destruction(Restore_original_framebuffer_on_destruction&&) = delete;
        Restore_original_framebuffer_on_destruction&
            operator=(Restore_original_framebuffer_on_destruction const&) = delete;
        Restore_original_framebuffer_on_destruction& operator=(Restore_original_framebuffer_on_destruction&&) = delete;
        ~Restore_original_framebuffer_on_destruction() noexcept {
            gl::BindFrameBuffer(GL_DRAW_FRAMEBUFFER, original_draw_fbo);
            gl::BindFrameBuffer(GL_READ_FRAMEBUFFER, original_read_fbo);
        }
    };

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
                    Restore_original_framebuffer_on_destruction restore_fbos;

                    gl::Frame_buffer rv;

                    // configure main FBO
                    gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                    gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color0);
                    gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, color1.type, color1, 0);
                    gl::FramebufferRenderbuffer(
                        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth24stencil8);

                    // check it's OK
                    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                    return rv;
                }()} {
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
                    Restore_original_framebuffer_on_destruction restore_fbos;

                    gl::Frame_buffer rv;

                    // configure non-MSXAA fbo
                    gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                    gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.type, tex, 0);

                    // check non-MSXAA OK
                    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                    return rv;
                }()} {
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
                    gl::TextureParameteri(rv, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
                    gl::TextureParameteri(rv, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
                    return rv;
                }()},
                fbo{[this]() {
                    Restore_original_framebuffer_on_destruction restore_fbos;

                    gl::Frame_buffer rv;
                    gl::BindFrameBuffer(GL_FRAMEBUFFER, rv);
                    gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex.type, tex, 0);

                    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

                    return rv;
                }()} {
            }
        };

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
            color1_resolved{w, h, GL_RGBA} {
        }
    };
}

namespace osmv {
    // internal renderer implementation details
    struct Renderer_impl final {

        struct {
            Gouraud_mrt_shader gouraud;
            Normals_shader normals;
            Plain_texture_shader plain_texture;
            Edge_detection_shader edge_detection_shader;
            Skip_msxaa_blitter_shader skip_msxaa_shader;
        } shaders;

        // debug quad
        gl::Array_bufferT<osmv::Shaded_textured_vert> quad_vbo = osmv::shaded_textured_quad_verts;
        gl::Vertex_array edge_detection_quad_vao = Edge_detection_shader::create_vao(quad_vbo);
        gl::Vertex_array skip_msxaa_quad_vao = Skip_msxaa_blitter_shader::create_vao(quad_vbo);
        gl::Vertex_array pts_quad_vao = Plain_texture_shader::create_vao(quad_vbo);

        // floor texture
        struct {
            gl::Array_bufferT<osmv::Shaded_textured_vert> vbo = []() {
                auto copy = osmv::shaded_textured_quad_verts;
                for (osmv::Shaded_textured_vert& st : copy) {
                    st.texcoord *= 25.0f;  // make chequers smaller
                }
                return gl::Array_bufferT<osmv::Shaded_textured_vert>{copy};
            }();

            gl::Vertex_array vao = Plain_texture_shader::create_vao(vbo);
            gl::Texture_2d floor_texture = osmv::generate_chequered_floor_texture();
            glm::mat4 model_mtx = []() {
                glm::mat4 rv = glm::identity<glm::mat4>();
                rv = glm::rotate(rv, osmv::pi_f / 2, {-1.0, 0.0, 0.0});
                rv = glm::scale(rv, {100.0f, 100.0f, 0.0f});
                return rv;
            }();
        } floor;

        // other OpenGL (GPU) buffers used by the renderer
        Renderer_buffers buffers;

        // internal (mutable) copy of the meshes being drawn
        std::vector<Mesh_instance> meshes_copy;

        Renderer_impl(int w, int h, int samples) : buffers{w, h, samples} {
        }
    };
}

int osmv::globally_allocate_mesh(osmv::Untextured_vert const* verts, size_t n) {
    assert(global_meshes.size() < std::numeric_limits<int>::max());
    int meshid = static_cast<int>(global_meshes.size());
    global_meshes.emplace_back(verts, n);
    return meshid;
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
osmv::Raw_renderer::Raw_renderer(int w, int h, int samples) : state(new Renderer_impl(w, h, samples)) {
}

osmv::Raw_renderer::~Raw_renderer() noexcept {
    delete state;
}

void osmv::Raw_renderer::resize(int w, int h, int samples) {
    Renderer_buffers& b = state->buffers;
    if (w != b.w or h != b.h or samples != b.samples) {
        b = Renderer_buffers{w, h, samples};
    }
}

void osmv::Raw_renderer::draw(Mesh_instance const* ms, size_t n) {
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

    // copy the provided geometry, because this implementation will need to reorganize the
    // geometry list
    state->meshes_copy.assign(ms, ms + n);
    std::vector<Mesh_instance>& meshes = state->meshes_copy;
    Renderer_buffers& buffers = state->buffers;

    // step 1: partition the mesh instances into those that are solid colored and those that
    //         require alpha blending
    //
    // ideally, rendering would follow the `painter's algorithm` and draw each pixel back-to-front.
    // We don't do that here, because constructing the various octrees, bsp's etc. to do that would
    // add a bunch of complexity CPU-side that's entirely unnecessary for such basic scenes. Also,
    // OpenGL benefits from the entirely opposite algorithm (render front-to-back) because it
    // uses depth testing as part of the "early fragment test" phase. Isn't low-level rendering
    // fun? :)
    //
    // so the hack here is to indiscriminately render all solid geometry first followed by
    // indiscriminately rendering all alpha-blended geometry. The edge-case failure mode is that
    // alpha blended geometry, itself, should be rendered back-to-front because alpha-blended
    // geometry can be intercalated or occluding other alpha-blended geometry.
    auto sort_by_alpha_then_meshid = [](Mesh_instance const& a, Mesh_instance const& b) {
        return a.rgba.a < b.rgba.a or a._meshid < b._meshid;
    };
    std::sort(meshes.begin(), meshes.end(), sort_by_alpha_then_meshid);

    // step 3: bind to an off-screen framebuffer object (FBO)
    //
    // drawing into this FBO writes to textures that the user can't see, but that can
    // be sampled by downstream shaders
    //
    // this draw call shouldn't affect which FBO was bound to before the call was made,
    // so save the original (input/output) FBOs
    GLint original_draw_fbo;
    GLint original_read_fbo;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &original_draw_fbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &original_read_fbo);

    gl::BindFrameBuffer(GL_FRAMEBUFFER, buffers.scene.fbo);

    // step 4: clear the scene FBO's draw buffers for a new draw call
    //
    //   - COLOR0: main scene render: fill in background
    //   - COLOR1: RGBA passthrough (selection logic + rim alpa): blank out all channels
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::ClearColor(background_rgba);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl::DrawBuffer(GL_COLOR_ATTACHMENT1);
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT);

    // step 5: render the scene to the FBO using a multiple-render-target (MRT)
    //         multisampled (MSXAAed) shader. FBO outputs are:
    //
    // - COLOR0: main target: multisampled scene geometry
    //     - the input color is Gouraud-shaded based on light parameters etc.
    // - COLOR1: RGBA passthrough: written to output as-is
    //     - the input color encodes the selected component index (RGB) and the rim
    //       alpha (A). It's used in downstream steps
    if (true) {
        GLenum original_poly_mode = gl::GetEnum(GL_POLYGON_MODE);
        glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);

        // draw state geometry with MRT shader
        gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1);

        // blending:
        // COLOR0 should be blended: OpenSim scenes can contain blending
        // COLOR1 should not be blended: it's a value for the top-most fragment
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnablei(GL_BLEND, 0);
        glDisablei(GL_BLEND, 1);

        Gouraud_mrt_shader& shader = state->shaders.gouraud;
        gl::UseProgram(shader.program);

        gl::Uniform(shader.uProjMat, projection_matrix);
        gl::Uniform(shader.uViewMat, view_matrix);
        gl::Uniform(shader.uLightPos, light_pos);
        gl::Uniform(shader.uLightColor, light_rgb);
        gl::Uniform(shader.uViewPos, view_pos);

        // upload instances
        gl::Array_bufferT<osmv::Mesh_instance>& mis = get_mi_storage();
        mis.assign(meshes.data(), meshes.data() + meshes.size());

        // batch drawcalls by meshid
        size_t pos = 0;
        while (pos < meshes.size()) {
            int id = meshes[pos]._meshid;
            size_t end = pos + 1;

            while (end < meshes.size() and meshes[end]._meshid == id) {
                ++end;
            }

            Mesh_on_gpu& md = global_mesh_lookup(id);
            gl::BindVertexArray(md.main_vao);
            gl::BindBuffer(mis);
            Mat4Pointer(shader.aModelMat, pos * sizeof(osmv::Mesh_instance) + offsetof(osmv::Mesh_instance, transform));
            Mat4Pointer(
                shader.aNormalMat, pos * sizeof(osmv::Mesh_instance) + offsetof(osmv::Mesh_instance, _normal_xform));
            Vec4Pointer(shader.aRgba0, pos * sizeof(osmv::Mesh_instance) + offsetof(osmv::Mesh_instance, rgba));
            Vec4Pointer(shader.aRgba1, pos * sizeof(osmv::Mesh_instance) + offsetof(osmv::Mesh_instance, _passthrough));
            gl::BindBuffer(md.vbo);
            glDrawArraysInstanced(GL_TRIANGLES, 0, md.sizei(), end - pos);
            gl::BindVertexArray();

            pos = end;
        }

        // nothing else in the scene uses blending
        glDisable(GL_BLEND);

        // (optional): draw a chequered floor
        if (show_floor) {
            // only drawn to COLOR0, doesn't contain any passthrough info
            gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
            Plain_texture_shader& pts = state->shaders.plain_texture;
            gl::UseProgram(pts.p);

            gl::Uniform(pts.projMat, projection_matrix);
            gl::Uniform(pts.viewMat, view_matrix);
            gl::Uniform(pts.modelMat, state->floor.model_mtx);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(state->floor.floor_texture);
            gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(pts.uSamplerMultiplier, gl::identity_val);

            gl::BindVertexArray(state->floor.vao);
            gl::DrawArrays(GL_TRIANGLES, 0, state->floor.vbo.sizei());
            gl::BindVertexArray();
        }

        glPolygonMode(GL_FRONT_AND_BACK, original_poly_mode);

        // (optional): render scene normals
        //
        // if the caller wants to view normals, pump the scene through a specialized shader
        // that draws normals as lines in COLOR0
        if (show_mesh_normals) {
            // only drawn to COLOR0, doesn't contain any passthrough info
            gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
            Normals_shader& ns = state->shaders.normals;
            gl::UseProgram(ns.program);

            gl::Uniform(ns.uProjMat, projection_matrix);
            gl::Uniform(ns.uViewMat, view_matrix);

            for (Mesh_instance const& m : meshes) {
                gl::Uniform(ns.uModelMat, m.transform);
                gl::Uniform(ns.uNormalMat, m._normal_xform);

                Mesh_on_gpu& md = global_mesh_lookup(m._meshid);
                gl::BindVertexArray(md.normal_vao);
                gl::DrawArrays(GL_TRIANGLES, 0, md.sizei());
            }
            gl::BindVertexArray();
        }
    }

    // step 6: figure out if the mouse is hovering over anything
    //
    // in the previous draw call, COLOR1's RGB channels encoded the index of the mesh instance.
    // Extracting that pixel value (without MSXAA blending) and decoding it back into an index
    // makes it possible to figure out what OpenSim::Component the mouse is over without requiring
    // complex spatial algorithms
    if (passthrough_hittest.enabled) {
        // (temporarily) set the OpenGL viewport to a small square around the hit testing
        // location
        //
        // this causes the subsequent draw call to only run the fragment shader around where
        // we actually care about
        glViewport(passthrough_hittest.x - 1, passthrough_hittest.y - 1, 3, 3);

        // bind to a non-MSXAAed FBO
        gl::BindFrameBuffer(GL_FRAMEBUFFER, buffers.skip_msxaa.fbo);
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

        // use a specialized (multisampling) shader to blit exactly one non-blended AA sample from
        // COLOR1 to the non-MSXAAed output
        //
        // by skipping MSXAA, every value in this output should to be exactly the same as the
        // value provided during drawing. Resolving MSXAA could potentially blend adjacent
        // values together, resulting in junk.
        Skip_msxaa_blitter_shader& shader = state->shaders.skip_msxaa_shader;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(buffers.scene.color1);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(state->skip_msxaa_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, state->quad_vbo.sizei());
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
        if (passthrough_hittest.optimized) {
            int reader = buffers.pbo_idx % static_cast<int>(buffers.pbos.size());
            int mapper = (buffers.pbo_idx + 1) % static_cast<int>(buffers.pbos.size());

            // launch asynchronous request for this frame's pixel
            gl::BindBuffer(buffers.pbos[static_cast<size_t>(reader)]);
            glReadPixels(passthrough_hittest.x, passthrough_hittest.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

            // synchrnously read *last frame's* pixel
            gl::BindBuffer(buffers.pbos[static_cast<size_t>(mapper)]);
            GLubyte* src = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));

            // these aren't applicable if using delayed hit testing
            passthrough_hittest.cur_frame_passthrough[0] = 0x00;
            passthrough_hittest.cur_frame_passthrough[1] = 0x00;

            passthrough_hittest.prev_frame_passthrough[0] = src[0];
            passthrough_hittest.prev_frame_passthrough[1] = src[1];

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

            // flip PBOs ready for next frame
            buffers.pbo_idx = (buffers.pbo_idx + 1) % static_cast<int>(buffers.pbos.size());
        } else {
            // slow mode: synchronously read the current frame's pixel under the cursor
            //
            // this is kept here so that people can try it out if selection logic is acting
            // bizzarely (e.g. because it is delayed one frame)

            GLubyte rgba[4]{};
            glReadPixels(passthrough_hittest.x, passthrough_hittest.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

            passthrough_hittest.prev_frame_passthrough[0] = passthrough_hittest.cur_frame_passthrough[0];
            passthrough_hittest.prev_frame_passthrough[1] = passthrough_hittest.cur_frame_passthrough[1];

            passthrough_hittest.cur_frame_passthrough[0] = rgba[0];
            passthrough_hittest.cur_frame_passthrough[1] = rgba[1];
        }
    }

    // step 8: resolve MSXAA in COLOR1
    //
    // "resolve" (i.e. blend) the MSXAA samples from the scene render into non-MSXAAed textures
    // that downstream shaders can sample normally.
    //
    // This is done separately because an intermediate step (decoding pixel colors into Component
    // indices) has to be MSXAA-aware (i.e. using a multisampling sampler) because it's performing
    // pixel decoding steps
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, buffers.scene.fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffers.color1_resolved.fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, buffers.w, buffers.h, 0, 0, buffers.w, buffers.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // step 9: blit COLOR0 to output
    //
    // COLOR0 is now "done", so can be written to the output
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, buffers.scene.fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, original_draw_fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, buffers.w, buffers.h, 0, 0, buffers.w, buffers.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // step 10: blend rims over the output (if necessary)
    //
    // COLOR0's alpha channel contains *filled in shapes* for each element in the scene that
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
    // "zoom out" as if they were "in the scene"). However, GPUs are *very* efficient at running
    // branchless algorithms over a screen, so it isn't as expensive as you think, and the utility
    // of having rims in worldspace is limited.
    //
    // composition is also done here because this single draw call has all the information
    // necessary to do it
    if (draw_rims) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, original_draw_fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        Edge_detection_shader& shader = state->shaders.edge_detection_shader;
        gl::UseProgram(shader.p);

        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(buffers.color1_resolved.tex);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::Uniform(shader.uRimRgba, rim_rgba);
        gl::Uniform(shader.uRimThickness, rim_thickness);

        glEnable(GL_BLEND);  // rims can have alpha
        gl::BindVertexArray(state->edge_detection_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, state->quad_vbo.sizei());
        gl::BindVertexArray();
        glDisable(GL_BLEND);
    }

    // (optional): render debug quads
    //
    // if the application is rendering in debug mode, then render quads for the intermediate
    // buffers (selection etc.) because it's handy for debugging
    if (draw_debug_quads) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, original_draw_fbo);

        Plain_texture_shader& pts = state->shaders.plain_texture;
        gl::UseProgram(pts.p);

        gl::Uniform(pts.projMat, gl::identity_val);
        gl::Uniform(pts.viewMat, gl::identity_val);
        gl::BindVertexArray(state->pts_quad_vao);

        // COLOR1 quad (RGB)
        if (true) {
            glm::mat4 row1 = []() {
                glm::mat4 m = glm::identity<glm::mat4>();
                m = glm::translate(m, glm::vec3{0.80f, 0.80f, -1.0f});  // move to [+0.6, +1.0] in x
                m = glm::scale(m, glm::vec3{0.20f});  // so it becomes [-0.2, +0.2]
                return m;
            }();

            gl::Uniform(pts.modelMat, row1);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(buffers.color1_resolved.tex);
            gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(pts.uSamplerMultiplier, gl::identity_val);
            gl::DrawArrays(GL_TRIANGLES, 0, state->quad_vbo.sizei());
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

            gl::Uniform(pts.modelMat, row2);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(buffers.color1_resolved.tex);
            gl::Uniform(pts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(pts.uSamplerMultiplier, alpha2rgb);
            gl::DrawArrays(GL_TRIANGLES, 0, state->quad_vbo.sizei());
        }

        gl::BindVertexArray();
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, original_read_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, original_draw_fbo);
}

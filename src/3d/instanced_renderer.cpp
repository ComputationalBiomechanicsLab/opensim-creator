#include "instanced_renderer.hpp"

#include "src/app.hpp"
#include "src/assertions.hpp"
#include "src/3d/gouraud_mrt_shader.hpp"
#include "src/3d/normals_shader.hpp"
#include "src/3d/edge_detection_shader.hpp"
#include "src/3d/colormapped_plain_texture_shader.hpp"

using namespace osc;

namespace {
    gl::Render_buffer mk_renderbuffer(int samples, GLenum format, int w, int h) {
        gl::Render_buffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format, w, h);
        return rv;
    }

    struct Render_target final {
        glm::ivec2 dims;
        int samples;

        // internally used for initial render pass
        gl::Render_buffer scene_msxaa_rb;
        gl::Render_buffer rims_msxaa_rb;
        gl::Render_buffer depth24stencil8_msxaa_rb;
        gl::Frame_buffer render_msxaa_fbo;

        // internally used to blit the solid rims (before edge-detection) into
        // a cheaper-to-sample not-multisampled texture
        gl::Texture_2d rims_tex;
        gl::Frame_buffer rims_tex_fbo;

        // these are the actual outputs
        gl::Texture_2d output_tex;
        gl::Texture_2d output_depth24stencil8_tex;
        gl::Frame_buffer output_fbo;

        Render_target(glm::ivec2 dims_, int samples_) :
            dims{dims_},
            samples{samples_},

            scene_msxaa_rb{mk_renderbuffer(samples, GL_RGBA, dims.x, dims.y)},
            rims_msxaa_rb{mk_renderbuffer(samples, GL_RED, dims.x, dims.y)},
            depth24stencil8_msxaa_rb{mk_renderbuffer(samples, GL_DEPTH24_STENCIL8, dims.x, dims.y)},
            render_msxaa_fbo{[this]() {
                gl::Frame_buffer rv;
                gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, scene_msxaa_rb);
                gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, rims_msxaa_rb);
                gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, depth24stencil8_msxaa_rb);
                gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
                return rv;
            }()},

            rims_tex{[this]() {
                gl::Texture_2d rv;
                gl::BindTexture(rv);
                gl::TexImage2D(rv.type, 0, GL_RED, dims.x, dims.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
                gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
                gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
                return rv;
            }()},
            rims_tex_fbo{[this]() {
                gl::Frame_buffer rv;
                gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, rims_tex, 0);
                gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
                return rv;
            }()},

            output_tex{[this]() {
                gl::Texture_2d rv;
                gl::BindTexture(rv);
                gl::TexImage2D(rv.type, 0, GL_RGBA, dims.x, dims.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
                gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
                return rv;
            }()},
            output_depth24stencil8_tex{[this]() {
                gl::Texture_2d rv;
                gl::BindTexture(rv);
                // https://stackoverflow.com/questions/27535727/opengl-create-a-depth-stencil-texture-for-reading
                gl::TexImage2D(rv.type, 0, GL_DEPTH24_STENCIL8, dims.x, dims.y, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
                return rv;
            }()},
            output_fbo{[this]() {
                gl::Frame_buffer rv;
                gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, output_tex, 0);
                gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, output_depth24stencil8_tex, 0);
                gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
                return rv;
            }()} {
        }
    };

    void configure_vao(Gouraud_mrt_shader& shader, Mesh_instance_meshdata& meshdata, gl::Vertex_array& vao) {
        size_t stride;
        size_t offset_pos;
        size_t offset_normal;
        size_t offset_texcoord;

        if (meshdata.is_textured) {
            stride = sizeof(Textured_vert);
            offset_pos = offsetof(Textured_vert, pos);
            offset_normal = offsetof(Textured_vert, normal);
            offset_texcoord = offsetof(Textured_vert, texcoord);
        } else {
            stride = sizeof(Untextured_vert);
            offset_pos = offsetof(Untextured_vert, pos);
            offset_normal = offsetof(Untextured_vert, normal);
            offset_texcoord = -1;
        }

        gl::BindVertexArray(vao);

        // bind vertex data to (non-instanced) attrs
        gl::BindBuffer(meshdata.verts);
        gl::VertexAttribPointer(shader.aLocation, false, stride, offset_pos);
        gl::EnableVertexAttribArray(shader.aLocation);
        gl::VertexAttribPointer(shader.aNormal, false, stride, offset_normal);
        gl::EnableVertexAttribArray(shader.aNormal);
        if (meshdata.is_textured) {
            gl::VertexAttribPointer(shader.aTexCoord, false, stride, offset_texcoord);
            gl::EnableVertexAttribArray(shader.aTexCoord);
        }

        // bind EBO
        gl::BindBuffer(meshdata.indices);

        // bind instance data
        gl::BindBuffer(meshdata.instances);

        gl::VertexAttribPointer(shader.aModelMat, false, sizeof(Mesh_instance), offsetof(Mesh_instance, model_xform));
        gl::VertexAttribDivisor(shader.aModelMat, 1);
        gl::EnableVertexAttribArray(shader.aModelMat);

        gl::VertexAttribPointer(shader.aNormalMat, false, sizeof(Mesh_instance), offsetof(Mesh_instance, normal_xform));
        gl::VertexAttribDivisor(shader.aNormalMat, 1);
        gl::EnableVertexAttribArray(shader.aNormalMat);

        // note: RGB is normalized CPU side ([0x00, 0xff]) and needs to be unpacked
        // into a float
        gl::VertexAttribPointer<gl::glsl::vec4, GL_UNSIGNED_BYTE>(shader.aRgba0, true, sizeof(Mesh_instance), offsetof(Mesh_instance, rgba));
        gl::VertexAttribDivisor(shader.aRgba0, 1);
        gl::EnableVertexAttribArray(shader.aRgba0);

        // note: RimIntensity is normalized from its CPU byte value into a float
        gl::VertexAttribPointer<gl::glsl::float_, GL_UNSIGNED_BYTE>(shader.aRimIntensity, true, sizeof(Mesh_instance), offsetof(Mesh_instance, rim_intensity));
        gl::VertexAttribDivisor(shader.aRimIntensity, 1);
        gl::EnableVertexAttribArray(shader.aRimIntensity);

        gl::BindVertexArray();
    }
}

struct osc::Instanced_renderer::Impl final {
    Gouraud_mrt_shader gouraud;
    Edge_detection_shader edge_detect;
    Render_target rt;

    gl::Array_buffer<Textured_vert> quad_vbo{generate_banner<Textured_mesh>().verts};
    gl::Vertex_array edgedetect_vao = [this]() {
        gl::Vertex_array rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(quad_vbo);
        gl::VertexAttribPointer(edge_detect.aPos, false, sizeof(Textured_vert), offsetof(Textured_vert, pos));
        gl::EnableVertexAttribArray(edge_detect.aPos);
        gl::VertexAttribPointer(edge_detect.aTexCoord, false, sizeof(Textured_vert), offsetof(Textured_vert, texcoord));
        gl::EnableVertexAttribArray(edge_detect.aTexCoord);
        return rv;
    }();

    Impl(glm::ivec2 dims, int samples) :
        gouraud{},
        rt{dims, samples} {
    }
};

// public API

osc::Mesh_instance_meshdata::Mesh_instance_meshdata(osc::Untextured_mesh const& m) :
    verts{reinterpret_cast<GLubyte const*>(m.verts.data()), sizeof(Untextured_vert) * m.verts.size()},
    indices{m.indices},
    instances{},
    vaos{},
    is_textured{false} {
}

osc::Mesh_instance_meshdata::Mesh_instance_meshdata(osc::Textured_mesh const& m) :
    verts{reinterpret_cast<GLubyte const*>(m.verts.data()), sizeof(Textured_vert) * m.verts.size()},
    indices{m.indices},
    instances{},
    vaos{},
    is_textured{true} {
}

osc::Instanced_renderer::Instanced_renderer() :
    m_Impl{new Impl{glm::ivec2{1, 1}, 1}} {
}

osc::Instanced_renderer::Instanced_renderer(glm::ivec2 dims, int samples) :
    m_Impl{new Impl{dims, samples}} {
}

osc::Instanced_renderer::~Instanced_renderer() noexcept = default;

glm::ivec2 osc::Instanced_renderer::dims() const noexcept {
    return m_Impl->rt.dims;
}

glm::vec2 osc::Instanced_renderer::dimsf() const noexcept {
    return m_Impl->rt.dims;
}

void osc::Instanced_renderer::set_dims(glm::ivec2 d) {
    Render_target& rt = m_Impl->rt;

    if (rt.dims == d) {
        return;  // no change
    }

    // else: remake buffers
    rt = Render_target{d, rt.samples};
}

float osc::Instanced_renderer::aspect_ratio() const noexcept {
    glm::ivec2 const& dimsi = m_Impl->rt.dims;
    glm::vec2 dimsf{dimsi};
    return dimsf.x/dimsf.y;

}

int osc::Instanced_renderer::msxaa_samples() const noexcept {
    return m_Impl->rt.samples;
}

void osc::Instanced_renderer::set_msxaa_samples(int samps) {
    Render_target& rt = m_Impl->rt;

    if (rt.samples == samps) {
        return; // no change
    }

    // else: remake buffers
    rt = Render_target{rt.dims, samps};
}

void osc::Instanced_renderer::render(Render_params const& p, Mesh_instance_drawlist& dl) {
    Impl& impl = *m_Impl;
    Render_target& rt = impl.rt;

    // global IDs for VAOs used by the renderer
    static int const g_GouraudVaoID = make_id<gl::Vertex_array>();

    // ensure VAOs are initialized each meshdata's hashtable
    for (auto& data_ptr : dl.meshes) {
        Mesh_instance_meshdata& data = *data_ptr;

        auto [it, inserted] = data.vaos.try_emplace(g_GouraudVaoID);
        if (inserted) {
            gl::Vertex_array& vao = it->second;
            configure_vao(impl.gouraud, data, vao);
        }
    }

    gl::Viewport(0, 0, rt.dims.x, rt.dims.y);
    gl::BindFramebuffer(GL_FRAMEBUFFER, rt.render_msxaa_fbo);

    // clear render buffers
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::ClearColor(p.background_rgba);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT1);
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT);

    // handle whether to draw in wireframe mode or not
    GLenum original_poly_mode = gl::GetEnum(GL_POLYGON_MODE);
    if (p.flags & DrawcallFlags_WireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // draw scene
    if (p.flags & DrawcallFlags_DrawSceneGeometry) {
        Gouraud_mrt_shader& shader = impl.gouraud;

        // setup per-render params
        gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, p.projection_matrix);
        gl::Uniform(shader.uViewMat, p.view_matrix);
        gl::Uniform(shader.uLightDir, p.light_dir);
        gl::Uniform(shader.uLightColor, p.light_rgb);
        gl::Uniform(shader.uViewPos, p.view_pos);
        gl::Uniform(shader.uIsShaded, true);
        gl::Uniform(shader.uSkipVP, false);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnablei(GL_BLEND, 0);  // COLOR0
        glDisablei(GL_BLEND, 1);  // COLOR1

        if (p.flags & DrawcallFlags_UseInstancedRenderer) {
            // perform instanced render

            size_t pos = 0;
            size_t ninsts = dl.instances.size();
            Mesh_instance const* insts = dl.instances.data();

            // iterate through all instances
            while (pos < ninsts) {
                auto meshidx = insts[pos].meshidx;
                auto texidx = insts[pos].texidx;

                // group adjacent elements with the same mesh + texture
                size_t end = pos + 1;
                while (end < ninsts && insts[end].meshidx == meshidx && insts[end].texidx == texidx) {
                    ++end;
                }

                // setup texture (if necessary)
                if (texidx >= 0) {
                    gl::Texture_2d& t = *dl.textures[texidx];
                    gl::Uniform(shader.uIsTextured, true);
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(t);
                    gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
                } else {
                    gl::Uniform(shader.uIsTextured, false);
                }

                // upload instance data to GPU
                Mesh_instance_meshdata& gpudata = *dl.meshes[meshidx];
                gpudata.instances.assign(insts + pos, end - pos);

                // draw
                gl::BindVertexArray(gpudata.vaos.at(g_GouraudVaoID));
                glDrawElementsInstanced(
                    GL_TRIANGLES,
                    gpudata.indices.sizei(),
                    gl::index_type(gpudata.indices),
                    nullptr,
                    static_cast<GLsizei>(end - pos));
                gl::BindVertexArray();

                pos = end;
            }

        } else {
            // perform non-instanced render
            for (auto& instance : dl.instances) {

                // setup texture (if necessary)
                if (instance.texidx >= 0) {
                    gl::Texture_2d& t = *dl.textures[instance.texidx];

                    gl::Uniform(shader.uIsTextured, true);
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(t);
                    gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
                } else {
                    gl::Uniform(shader.uIsTextured, false);
                }

                // upload instance data to GPU
                Mesh_instance_meshdata& data = *dl.meshes[instance.meshidx];
                data.instances.assign(&instance, 1);

                // draw
                gl::BindVertexArray(data.vaos.at(g_GouraudVaoID));
                glDrawElementsInstanced(
                    GL_TRIANGLES,
                    data.indices.sizei(),
                    gl::index_type(data.indices),
                    nullptr,
                    1);
                gl::BindVertexArray();
            }
        }
    }

    // reset wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, original_poly_mode);

    // blit scene to output texture
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, rt.render_msxaa_fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, rt.output_fbo);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::BlitFramebuffer(0, 0, rt.dims.x, rt.dims.y, 0, 0, rt.dims.x, rt.dims.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);


    // handle rim highlights (if necessary)
    if (p.flags & DrawcallFlags_DrawRims) {

        // blit rims from MSXAAed (expensive to sample) texture to a standard
        // not-MSXAAed texture
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, rt.render_msxaa_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, rt.rims_tex_fbo);
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, rt.dims.x, rt.dims.y, 0, 0, rt.dims.x, rt.dims.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // set shader to write directly to output
        gl::BindFramebuffer(GL_FRAMEBUFFER, rt.output_fbo);
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

        // setup edge-detection shader
        Edge_detection_shader& shader = impl.edge_detect;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(rt.rims_tex);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::Uniform(shader.uRimRgba, {1.0f, 0.4f, 0.0f, 0.85f});
        gl::Uniform(shader.uRimThickness, 5.0f / std::max(rt.dims.x, rt.dims.y));

        // draw edges, directly writing into output texture
        gl::Enable(GL_BLEND);
        gl::Disable(GL_DEPTH_TEST);
        gl::BindVertexArray(impl.edgedetect_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, impl.quad_vbo.sizei());
        gl::BindVertexArray();
        gl::Enable(GL_DEPTH_TEST);
    }

    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
}

gl::Texture_2d const& osc::Instanced_renderer::output_texture() const noexcept {
    return m_Impl->rt.output_tex;
}

gl::Texture_2d& osc::Instanced_renderer::output_texture() noexcept {
    return m_Impl->rt.output_tex;
}

gl::Texture_2d const& osc::Instanced_renderer::depth_texture() const noexcept {
    return m_Impl->rt.output_depth24stencil8_tex;

}

gl::Texture_2d& osc::Instanced_renderer::depth_texture() {
    return m_Impl->rt.output_depth24stencil8_tex;
}

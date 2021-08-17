#include "instanced_renderer.hpp"

#include "src/3d/gl.hpp"
#include "src/3d/gl_glm.hpp"
#include "src/3d/shaders/gouraud_mrt_shader.hpp"
#include "src/3d/shaders/edge_detection_shader.hpp"
#include "src/3d/shaders/normals_shader.hpp"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <cstddef>
#include <vector>
#include <memory>
#include <stdexcept>

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
}

struct Untextured_vert final {
    glm::vec3 pos;
    glm::vec3 norm;
};

struct Textured_vert final {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
};

static gl::Vertex_array create_Gouraud_vao(
        Gouraud_mrt_shader& shader,
        gl::Array_buffer<GLubyte>& data,  // if `is_textured` then Textured_vert, else Untextured_vert
        gl::Element_array_buffer<GLushort>& ebo,
        gl::Array_buffer<Mesh_instance, GL_DYNAMIC_DRAW>& instances,
        bool is_textured) {

    size_t stride;
    size_t offset_pos;
    size_t offset_normal;
    size_t offset_texcoord;
    if (is_textured) {
        stride = sizeof(Textured_vert);
        offset_pos = offsetof(Textured_vert, pos);
        offset_normal = offsetof(Textured_vert, norm);
        offset_texcoord = offsetof(Textured_vert, uv);
    } else {
        stride = sizeof(Untextured_vert);
        offset_pos = offsetof(Untextured_vert, pos);
        offset_normal = offsetof(Untextured_vert, norm);
        offset_texcoord = -1;
    }

    gl::Vertex_array vao;
    gl::BindVertexArray(vao);

    // bind vertex data to (non-instanced) attrs
    gl::BindBuffer(data);
    gl::VertexAttribPointer(shader.aLocation, false, stride, offset_pos);
    gl::EnableVertexAttribArray(shader.aLocation);
    gl::VertexAttribPointer(shader.aNormal, false, stride, offset_normal);
    gl::EnableVertexAttribArray(shader.aNormal);
    if (is_textured) {
        gl::VertexAttribPointer(shader.aTexCoord, false, stride, offset_texcoord);
        gl::EnableVertexAttribArray(shader.aTexCoord);
    }

    // bind EBO
    gl::BindBuffer(ebo);

    // bind instance data
    gl::BindBuffer(instances);

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

    return vao;
}

static gl::Vertex_array create_Normals_vao(
        Normals_shader& shader,
        gl::Array_buffer<GLubyte>& vbo,  // if `is_textured` then Textured_vert, else Untextured_vert
        gl::Element_array_buffer<GLushort>& ebo) {

    static_assert(offsetof(Untextured_vert, pos) == offsetof(Textured_vert, pos));
    static_assert(offsetof(Untextured_vert, norm) == offsetof(Textured_vert, norm));

    gl::Vertex_array vao;
    gl::BindVertexArray(vao);
    gl::BindBuffer(vbo);
    gl::BindBuffer(ebo);
    gl::VertexAttribPointer(shader.aPos, false, sizeof(Untextured_vert), offsetof(Untextured_vert, pos));
    gl::EnableVertexAttribArray(shader.aPos);
    gl::VertexAttribPointer(shader.aNormal, false, sizeof(Untextured_vert), offsetof(Untextured_vert, norm));
    gl::EnableVertexAttribArray(shader.aNormal);
    gl::BindVertexArray();
    return vao;
}

// backend implementation of the meshdata, uploaded in a GPU-friendly format
struct osc::Refcounted_instance_meshdata::Impl final {
    gl::Array_buffer<GLubyte> data;
    gl::Element_array_buffer<GLushort> indices;
    gl::Array_buffer<Mesh_instance, GL_DYNAMIC_DRAW> instances;
    gl::Vertex_array gouraud_vao;
    gl::Vertex_array normals_vao;

    Impl(gl::Array_buffer<GLubyte> data_,
         gl::Element_array_buffer<GLushort> indices_,
         gl::Array_buffer<Mesh_instance, GL_DYNAMIC_DRAW> instances_,
         gl::Vertex_array gouraud_vao_,
         gl::Vertex_array normals_vao_) :

        data{std::move(data_)},
        indices{std::move(indices_)},
        instances{std::move(instances_)},
        gouraud_vao{std::move(gouraud_vao_)},
        normals_vao{std::move(normals_vao_)} {
    }
};

struct osc::Instanced_renderer::Impl final {
    std::vector<Untextured_vert> uv_swap;
    std::vector<Textured_vert> tv_swap;
    Gouraud_mrt_shader gouraud;
    Edge_detection_shader edge_detect;
    Normals_shader normals_shader;
    Render_target rt;

    gl::Array_buffer<Textured_vert> quad_vbo{[&]() {
        NewMesh m = gen_textured_quad();

        tv_swap.clear();
        tv_swap.resize(m.indices.size());
        for (size_t i = 0; i < m.indices.size(); ++i) {
            unsigned short idx = m.indices[i];
            Textured_vert& tv = tv_swap.emplace_back();
            tv.pos = m.verts[idx];
            tv.norm = m.normals[idx];
            tv.uv = m.texcoords[idx];
        }

        return gl::Array_buffer<Textured_vert>{tv_swap};
    }()};
    gl::Vertex_array edgedetect_vao = [this]() {
        gl::Vertex_array rv;
        gl::BindVertexArray(rv);
        gl::BindBuffer(quad_vbo);
        gl::VertexAttribPointer(edge_detect.aPos, false, sizeof(Textured_vert), offsetof(Textured_vert, pos));
        gl::EnableVertexAttribArray(edge_detect.aPos);
        gl::VertexAttribPointer(edge_detect.aTexCoord, false, sizeof(Textured_vert), offsetof(Textured_vert, uv));
        gl::EnableVertexAttribArray(edge_detect.aTexCoord);
        return rv;
    }();

    Impl(glm::ivec2 dims, int samples) : rt{dims, samples} {
    }

    gl::Array_buffer<GLubyte> upload_mesh(NewMesh const& nm) {
        if (nm.verts.size() != nm.normals.size()) {
            throw std::runtime_error{"mismatch between number of verts and number of normals in a mesh"};
        }

        if (nm.texcoords.empty()) {
            // pack into Untextured_vert
            uv_swap.clear();
            uv_swap.reserve(nm.verts.size());
            for (size_t i = 0; i < nm.verts.size(); ++i) {
                Untextured_vert& uv = uv_swap.emplace_back();
                uv.pos = nm.verts[i];
                uv.norm = nm.normals[i];
            }
            return {reinterpret_cast<GLubyte const*>(uv_swap.data()), sizeof(Untextured_vert) * uv_swap.size()};
        } else {
            // pack into Textured_vert
            tv_swap.clear();
            tv_swap.reserve(nm.verts.size());
            for (size_t i = 0; i < nm.verts.size(); ++i) {
                Textured_vert& tv = tv_swap.emplace_back();
                tv.pos = nm.verts[i];
                tv.norm = nm.normals[i];
                tv.uv = nm.texcoords[i];
            }
            return {reinterpret_cast<GLubyte const*>(tv_swap.data()), sizeof(Textured_vert) * tv_swap.size()};
        }
    }
};

// public API

osc::Refcounted_instance_meshdata::Refcounted_instance_meshdata(std::shared_ptr<Impl> impl) :
    m_Impl{impl} {
}

osc::Refcounted_instance_meshdata::~Refcounted_instance_meshdata() noexcept = default;

void osc::Mesh_instance_drawlist::clear() {
    instances.clear();
    meshes.clear();
    textures.clear();
}

osc::Instanced_renderer::Instanced_renderer() :
    m_Impl{new Impl{glm::ivec2{1, 1}, 1}} {
}

osc::Instanced_renderer::Instanced_renderer(glm::ivec2 dims, int samples) :
    m_Impl{new Impl{dims, samples}} {
}

osc::Instanced_renderer::~Instanced_renderer() noexcept = default;

Refcounted_instance_meshdata osc::Instanced_renderer::allocate(NewMesh const& m) {
    gl::Array_buffer<GLubyte> vbo = m_Impl->upload_mesh(m);
    gl::Element_array_buffer<GLushort> ebo{m.indices};
    gl::Array_buffer<Mesh_instance, GL_DYNAMIC_DRAW> instances;
    gl::Vertex_array vao = create_Gouraud_vao(m_Impl->gouraud, vbo, ebo, instances, !m.texcoords.empty());
    gl::Vertex_array normals_vao = create_Normals_vao(m_Impl->normals_shader, vbo, ebo);

    auto md = std::make_shared<Refcounted_instance_meshdata::Impl>(std::move(vbo), std::move(ebo), std::move(instances), std::move(vao), std::move(normals_vao));

    return Refcounted_instance_meshdata{md};
}

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

void osc::Instanced_renderer::render(Render_params const& p, Mesh_instance_drawlist const& dl) {
    Impl& impl = *m_Impl;
    Render_target& rt = impl.rt;

    gl::Viewport(0, 0, rt.dims.x, rt.dims.y);
    gl::BindFramebuffer(GL_FRAMEBUFFER, rt.render_msxaa_fbo);

    // clear render buffers
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::ClearColor(p.background_rgba);
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT1);
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(GL_COLOR_BUFFER_BIT);

    // set wireframe mode on if requested
    if (p.flags & DrawcallFlags_WireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
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
                Refcounted_instance_meshdata::Impl& gpudata = *dl.meshes[meshidx].m_Impl;
                gpudata.instances.assign(insts + pos, end - pos);

                // draw
                gl::BindVertexArray(gpudata.gouraud_vao);
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
                Refcounted_instance_meshdata::Impl& data = *dl.meshes[instance.meshidx].m_Impl;
                data.instances.assign(&instance, 1);

                // draw
                gl::BindVertexArray(data.gouraud_vao);
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

    if (p.flags & DrawcallFlags_ShowMeshNormals) {
        Normals_shader& shader = impl.normals_shader;
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, p.projection_matrix);
        gl::Uniform(shader.uViewMat, p.view_matrix);

        for (Mesh_instance const& mi : dl.instances) {
            gl::Uniform(shader.uModelMat, mi.model_xform);
            gl::Uniform(shader.uNormalMat, mi.normal_xform);
            Refcounted_instance_meshdata::Impl const& meshdata = *dl.meshes[mi.meshidx].m_Impl;
            gl::BindVertexArray(meshdata.normals_vao);
            gl::DrawElements(GL_TRIANGLES, meshdata.indices.sizei(), gl::index_type(meshdata.indices), nullptr);
        }
        gl::BindVertexArray();
    }

    if (p.flags & DrawcallFlags_WireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // blit scene to output texture
    gl::BindFramebuffer(GL_READ_FRAMEBUFFER, rt.render_msxaa_fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, rt.output_fbo);
    gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
    gl::BlitFramebuffer(0, 0, rt.dims.x, rt.dims.y, 0, 0, rt.dims.x, rt.dims.y, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);


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

gl::Frame_buffer const& osc::Instanced_renderer::output_fbo() const noexcept {
    return m_Impl->rt.output_fbo;
}

gl::Frame_buffer& osc::Instanced_renderer::output_fbo() noexcept {
    return m_Impl->rt.output_fbo;
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

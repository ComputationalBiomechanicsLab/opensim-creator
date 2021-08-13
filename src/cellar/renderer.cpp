/* TODO
 *         template<typename Vbo, typename T = typename Vbo::value_type>
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
        */


/* TODO
 *         template<typename Vbo, typename T = typename Vbo::value_type>
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
        */


/* TODO
 *         template<typename Vbo, typename T = typename Vbo::value_type>
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
        */

/*
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
*/


/* TODO
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
*/

/*
 *

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
        }*/






namespace {

    [[nodiscard]] constexpr bool optimal_orderering(Mesh_instance const& m1, Mesh_instance const& m2) {
        if (m1.texidx != m2.texidx) {
            // third, sort by texture, because even though we *could* render a batch of
            // instances with the same mesh in one draw call, some of those meshes might
            // be textured, and textures can't be instanced (so the drawcall must be split
            // into separate calls etc.)
            return m1.texidx < m2.texidx;
        } else {
            // fourth, sort by flags, because the flags can change a draw call (e.g.
            // although we are drawing the same mesh with the same texture, this
            // partiular *instance* should be drawn with GL_TRIANGLES or GL_POINTS)
            //
            // like textures, if the drawcall-affecting flags are different, we have
            // to split the drawcall (e.g. draw TRIANGLES then draw POINTS)
            return m1.flags < m2.flags;
        }
    }
}


osc::GPU_mesh::GPU_mesh(Untextured_mesh const& um) :
    verts(reinterpret_cast<GLubyte const*>(um.verts.data()), sizeof(Untextured_vert) * um.verts.size()),
    indices(um.indices),
    instances{},
    main_vao(Gouraud_mrt_shader::create_vao<decltype(verts), Untextured_vert>(verts, indices, instances)),
    normal_vao(Normals_shader::create_vao<decltype(verts), Untextured_vert>(verts)),
    is_textured{false} {
}

osc::GPU_mesh::GPU_mesh(Textured_mesh const& tm) :
    verts(reinterpret_cast<GLubyte const*>(tm.verts.data()), sizeof(Textured_vert) * tm.verts.size()),
    indices(tm.indices),
    instances{},
    main_vao(Gouraud_mrt_shader::create_vao<decltype(verts), Textured_vert>(verts, indices, instances)),
    normal_vao(Normals_shader::create_vao<decltype(verts), Textured_vert>(verts)),
    is_textured{true} {
}

osc::GPU_storage::GPU_storage() :
    // shaders
    shader_gouraud{new Gouraud_mrt_shader{}},
    shader_normals{new Normals_shader{}},
    shader_pts{new Plain_texture_shader{}},
    shader_cpts{new Colormapped_plain_texture_shader{}},
    shader_eds{new Edge_detection_shader{}},
    shader_skip_msxaa{new Skip_msxaa_blitter_shader{}} {

    // untextured preallocated meshes
    {
        Untextured_mesh utm;

        unit_sphere_triangles(utm);
        meshes.emplace_back(utm);
        simbody_sphere_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();

        simbody_cylinder_triangles(utm);
        meshes.emplace_back(utm);
        simbody_cylinder_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();

        simbody_brick_triangles(utm);
        meshes.emplace_back(utm);
        simbody_cube_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();

        generate_NxN_grid(25, utm);
        meshes.emplace_back(utm);
        grid_25x25_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();

        generate_y_line(utm);
        meshes.emplace_back(utm);
        yline_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();

        generate_cube_lines(utm);
        meshes.emplace_back(utm);
        cube_lines_idx = Meshidx::from_index(meshes.size() - 1);
        utm.clear();
    }

    // textured preallocated meshes
    {
        Textured_mesh tm;

        generate_floor_quad(tm);
        meshes.emplace_back(tm);
        floor_quad_idx = Meshidx::from_index(meshes.size() - 1);
        tm.clear();

        for (Textured_vert const& tv : g_ShadedTexturedQuadVerts) {
            tm.verts.push_back(tv);
        }
        generate_1to1_indices_for_verts(tm);
        meshes.emplace_back(tm);
        quad_idx = Meshidx::from_index(meshes.size() - 1);
        quad_vbo = gl::Array_buffer<Textured_vert>(tm.verts);
    }

    // preallocated textures
    textures.push_back(generate_chequered_floor_texture());
    chequer_idx = Texidx::from_index(textures.size() - 1);

    eds_quad_vao = Edge_detection_shader::create_vao(quad_vbo);
    skip_msxaa_quad_vao = Skip_msxaa_blitter_shader::create_vao(quad_vbo);
    pts_quad_vao = Plain_texture_shader::create_vao(quad_vbo);
    cpts_quad_vao = Colormapped_plain_texture_shader::create_vao(quad_vbo);
}

osc::GPU_storage::GPU_storage(GPU_storage&&) noexcept = default;

osc::GPU_storage& osc::GPU_storage::operator=(GPU_storage&&) noexcept = default;

osc::GPU_storage::~GPU_storage() noexcept = default;

osc::Render_target::Render_target(int w_, int h_, int samples_) :
    w{w_},
    h{h_},
    samples{samples_},

    scene_rgba{[this]() {
        gl::Render_buffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA, w, h);
        return rv;
    }()},

    scene_passthrough{[this]() {
        gl::Texture_2d_multisample rv;
        gl::BindTexture(rv);
        glTexImage2DMultisample(rv.type, samples, GL_RGB, w, h, GL_TRUE);
        return rv;
    }()},

    scene_depth24stencil8{[this]() {
        gl::Render_buffer rv;
        gl::BindRenderBuffer(rv);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, w, h);
        return rv;
    }()},

    scene_fbo{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, scene_rgba);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, scene_passthrough, 0);
        gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, scene_depth24stencil8);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    passthrough_nomsxaa{[this]() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        return rv;
    }()},

    passthrough_fbo{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, passthrough_nomsxaa, 0);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    passthrough_pbos{{
        {0x00, 0x00, 0x00, 0x00},  // rgba
        {0x00, 0x00, 0x00, 0x00},  // rgba
    }},

    passthrough_pbo_cur{0},  // 0/1

    scene_tex_resolved{[this]() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(rv.type, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
        return rv;
    }()},

    scene_fbo_resolved{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, scene_tex_resolved, 0);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    passthrough_tex_resolved{[this]() {
        gl::Texture_2d rv;
        gl::BindTexture(rv);
        gl::TexImage2D(rv.type, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // no mipmaps
        gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // no mipmaps
        return rv;
    }()},

    passthrough_fbo_resolved{[this]() {
        gl::Frame_buffer rv;
        gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
        gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, passthrough_tex_resolved, 0);
        gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
        return rv;
    }()},

    hittest_result{0x00, 0x00, 0x00} {
}

void osc::Render_target::reconfigure(int w_, int h_, int samples_) {
    if (w != w_ || h != h_ || samples != samples_) {
        *this = Render_target{w_, h_, samples_};
    }
}

void osc::optimize(Drawlist& drawlist) noexcept {
    for (auto& lst : drawlist._nonopaque_by_meshidx) {
        std::sort(lst.begin(), lst.end(), optimal_orderering);
    }
    for (auto& lst : drawlist._opaque_by_meshidx) {
        std::sort(lst.begin(), lst.end(), optimal_orderering);
    }
}

void osc::draw_scene(GPU_storage& storage, Render_params const& params, Drawlist const& drawlist, Render_target& out) {
    gl::Viewport(0, 0, out.w, out.h);

    // bind to an off-screen framebuffer object (FBO)
    //
    // drawing into this FBO writes to textures that the user can't see, but that can
    // be sampled by downstream shaders
    gl::BindFramebuffer(GL_FRAMEBUFFER, out.scene_fbo);

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
    if (params.flags & DrawcallFlags_DrawSceneGeometry) {
        Gouraud_mrt_shader& shader = *storage.shader_gouraud;

        gl::DrawBuffers(GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, params.projection_matrix);
        gl::Uniform(shader.uViewMat, params.view_matrix);
        gl::Uniform(shader.uLightDir, params.light_dir);
        gl::Uniform(shader.uLightColor, params.light_rgb);
        gl::Uniform(shader.uViewPos, params.view_pos);

        // blending:
        //     COLOR0 should be blended because OpenSim scenes can contain blending
        //     COLOR1 should never be blended: it's a value for the top-most fragment
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnablei(GL_BLEND, 0);
        glDisablei(GL_BLEND, 1);

        if (params.flags & DrawcallFlags_UseInstancedRenderer) {
            auto draw_els_with_same_meshidx = [&](std::vector<Mesh_instance> const& instances) {
                size_t nmeshes = instances.size();
                Mesh_instance const* meshes = instances.data();
                size_t pos = 0;

                // batch instances into as few drawcalls as possible
                while (pos < nmeshes) {
                    Meshidx meshidx = meshes[pos].meshidx;
                    Texidx texidx = meshes[pos].texidx;
                    Instance_flags flags = meshes[pos].flags;
                    size_t end = pos + 1;

                    while (end < nmeshes && meshes[end].texidx == texidx && meshes[end].flags == flags) {
                        ++end;
                    }

                    // [pos, end) contains instances with the same meshid + textureid + flags

                    // assign texture (if necessary)
                    if (texidx.is_valid()) {
                        gl::Uniform(shader.uIsTextured, true);
                        gl::ActiveTexture(GL_TEXTURE0);
                        gl::BindTexture(storage.textures[texidx.as_index()]);
                        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
                    } else {
                        gl::Uniform(shader.uIsTextured, false);
                    }

                    // assign flags
                    gl::Uniform(shader.uIsShaded, !flags.skip_shading());
                    gl::Uniform(shader.uSkipVP, flags.skip_vp());

                    // upload instance data
                    GPU_mesh& gm = storage.meshes[meshidx.as_index()];
                    gm.instances.assign(meshes + pos, end - pos);

                    // do drawcall
                    gl::BindVertexArray(gm.main_vao);
                    glDrawElementsInstanced(
                        flags.mode(), gm.indices.sizei(), gl::index_type(gm.indices), nullptr, static_cast<GLsizei>(end - pos));

                    pos = end;
                }
            };

            for (auto const& lst : drawlist._opaque_by_meshidx) {
                draw_els_with_same_meshidx(lst);
            }

            for (auto const& lst : drawlist._nonopaque_by_meshidx) {
                draw_els_with_same_meshidx(lst);
            }

            gl::BindVertexArray();
        } else {
            // perform (slower) one-drawcall-per-item rendering
            //
            // this is here mostly for perf comparison and debugging
            drawlist.for_each([&](Mesh_instance const& mi) {
                // texture-related stuff
                if (mi.texidx.is_valid()) {
                    gl::Uniform(shader.uIsTextured, true);
                    gl::ActiveTexture(GL_TEXTURE0);
                    gl::BindTexture(storage.textures[mi.texidx.as_index()]);
                    gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
                } else {
                    gl::Uniform(shader.uIsTextured, false);
                }

                // flag-related stuff
                gl::Uniform(shader.uIsShaded, !mi.flags.skip_shading());
                gl::Uniform(shader.uSkipVP, mi.flags.skip_vp());
                GLenum mode = mi.flags.mode();

                GPU_mesh& gm = storage.meshes[mi.meshidx.as_index()];
                gm.instances.assign(&mi, 1);
                gl::BindVertexArray(gm.main_vao);
                glDrawElementsInstanced(mode, gm.indices.sizei(), gl::index_type(gm.indices), nullptr, static_cast<GLsizei>(1));
                gl::BindVertexArray();
            });
        }

        glDisablei(GL_BLEND, 0);
    }

    glPolygonMode(GL_FRONT_AND_BACK, original_poly_mode);

    // (optional): render scene normals into COLOR0
    if (params.flags & DrawcallFlags_ShowMeshNormals) {
        Normals_shader& shader = *storage.shader_normals;
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::UseProgram(shader.program);
        gl::Uniform(shader.uProjMat, params.projection_matrix);
        gl::Uniform(shader.uViewMat, params.view_matrix);

        drawlist.for_each([&](Mesh_instance const& mi) {
            GPU_mesh& gm = storage.meshes[mi.meshidx.as_index()];
            gl::Uniform(shader.uModelMat, mi.model_xform);
            gl::Uniform(shader.uNormalMat, mi.normal_xform);
            gl::BindVertexArray(gm.normal_vao);
            gl::DrawArrays(GL_TRIANGLES, 0, gm.verts.sizei() / (gm.is_textured ? sizeof(Textured_vert) : sizeof(Untextured_vert)));
        });

        gl::BindVertexArray();
    }

    // perform passthrough hit testing
    //
    // in the previous draw call, COLOR1's RGB channels encoded arbitrary passthrough data
    // Extracting that pixel value (without MSXAA blending) and decoding it yields the
    // user-supplied data
    //
    // this makes it possible for renderer users (e.g. OpenSim model renderer) to encode
    // model information (e.g. "a component index") into screenspace

    out.hittest_result = Passthrough_data{0x00, 0x00, 0x00};

    if (params.hittest.x >= 0 &&
        params.hittest.y >= 0 &&
        params.flags & DrawcallFlags_PerformPassthroughHitTest) {

        // (temporarily) set the OpenGL viewport to a small square around the hit testing
        // location
        //
        // this causes the subsequent draw call to only run the fragment shader around where
        // we actually care about
        gl::Viewport(params.hittest.x - 1, params.hittest.y - 1, 3, 3);

        // bind to a non-MSXAAed FBO
        gl::BindFramebuffer(GL_FRAMEBUFFER, out.passthrough_fbo);
        gl::DrawBuffer(GL_COLOR_ATTACHMENT0);

        // use a specialized shader that is MSXAA-aware to blit exactly one non-blended AA
        // sample from COLOR1 to the output
        //
        // by deliberately avoiding MSXAA, every value in this output should be exactly the
        // same as the passthrough value provided by the caller
        Skip_msxaa_blitter_shader& shader = *storage.shader_skip_msxaa;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(out.scene_passthrough);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::BindVertexArray(storage.skip_msxaa_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        gl::BindVertexArray();

        // reset viewport
        glViewport(0, 0, out.w, out.h);

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

        if (params.flags & DrawcallFlags_UseOptimizedButDelayed1FrameHitTest) {
            size_t reader = out.passthrough_pbo_cur % out.passthrough_pbos.size();
            size_t mapper = (out.passthrough_pbo_cur + 1) % out.passthrough_pbos.size();

            // launch asynchronous request for this frame's pixel
            gl::BindBuffer(out.passthrough_pbos[reader]);
            glReadPixels(params.hittest.x, params.hittest.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

            // synchrnously read *last frame's* pixel
            gl::BindBuffer(out.passthrough_pbos[mapper]);
            GLubyte* src = static_cast<GLubyte*>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));

            // note: these values are the *last frame*'s
            out.hittest_result.b0 = src[0];
            out.hittest_result.b1 = src[1];
            out.hittest_result.rim_alpha = src[2];

            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

            // flip PBOs ready for next frame
            out.passthrough_pbo_cur = (out.passthrough_pbo_cur+ 1) % out.passthrough_pbos.size();
        } else {
            // slow mode: synchronously read the current frame's pixel under the cursor
            //
            // this is kept here so that people can try it out if selection logic is acting
            // bizzarely (e.g. because it is delayed one frame)

            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
            GLubyte rgba[3]{};
            glReadPixels(params.hittest.x, params.hittest.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgba);

            out.hittest_result.b0 = rgba[0];
            out.hittest_result.b1 = rgba[1];
            out.hittest_result.rim_alpha = rgba[2];
        }
    }

    // resolve MSXAA in COLOR0 to output texture
    //
    // "resolve" (i.e. blend) the MSXAA samples in COLOR0. We are "done" with
    // COLOR0. You might expect we can directly blit it to the output, but that
    // seems to explode with some OpenGL drivers (e.g. Intel iGPUs like UHD 620)
    {
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, out.scene_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, out.scene_fbo_resolved);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, out.w, out.h, 0, 0, out.w, out.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // resolve MSXAA in COLOR1
    //
    // "resolve" (i.e. blend) the MSXAA samples in COLOR1 into non-MSXAAed textures
    // that the edge-detection shader can sample normally
    {
        gl::BindFramebuffer(GL_READ_FRAMEBUFFER, out.scene_fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, out.passthrough_fbo_resolved);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        gl::BlitFramebuffer(0, 0, out.w, out.h, 0, 0, out.w, out.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // bind to output texture: all further drawing goes onto it
    gl::BindFramebuffer(GL_FRAMEBUFFER, out.scene_fbo_resolved);

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
        Edge_detection_shader& shader = *storage.shader_eds;
        gl::UseProgram(shader.p);
        gl::Uniform(shader.uModelMat, gl::identity_val);
        gl::Uniform(shader.uViewMat, gl::identity_val);
        gl::Uniform(shader.uProjMat, gl::identity_val);
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(out.passthrough_tex_resolved);
        gl::Uniform(shader.uSampler0, gl::texture_index<GL_TEXTURE0>());
        gl::Uniform(shader.uRimRgba, params.rim_rgba);

        float rim_thickness = 2.0f / std::max(out.w, out.h);

        gl::Uniform(shader.uRimThickness, rim_thickness);

        glEnable(GL_BLEND);  // rims can have alpha
        glDisable(GL_DEPTH_TEST);
        gl::BindVertexArray(storage.eds_quad_vao);
        gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        gl::BindVertexArray();
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

    // render debug quads onto output (if applicable)
    if (params.flags & DrawcallFlags_DrawDebugQuads) {
        Colormapped_plain_texture_shader& cpts = *storage.shader_cpts;
        gl::UseProgram(cpts.p);
        gl::BindVertexArray(storage.pts_quad_vao);

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
            gl::BindTexture(out.passthrough_tex_resolved);
            gl::Uniform(cpts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(cpts.uSamplerMultiplier, gl::identity_val);
            gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
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
            gl::BindTexture(out.passthrough_tex_resolved);
            gl::Uniform(cpts.uSampler0, gl::texture_index<GL_TEXTURE0>());
            gl::Uniform(cpts.uSamplerMultiplier, alpha2rgb);
            gl::DrawArrays(GL_TRIANGLES, 0, storage.quad_vbo.sizei());
        }

        gl::BindVertexArray();
    }

    // bind back to the original framebuffer (assumed to be window)
    gl::BindFramebuffer(GL_FRAMEBUFFER, gl::window_fbo);
}

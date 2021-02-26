#pragma once

namespace {
    struct Gouraud_mrt_shader;
    struct Normals_shader;
    struct Plain_texture_shader;
    struct Colormapped_plain_texture_shader;
    struct Edge_detection_shader;
    struct Skip_msxaa_blitter_shader;
}

namespace osmv {
    class Shader_cache final {
        struct Impl;
        Impl* impl;

    public:
        Shader_cache();
        Shader_cache(Shader_cache const&) = delete;
        Shader_cache(Shader_cache&&) = delete;
        Shader_cache& operator=(Shader_cache const&) = delete;
        Shader_cache& operator=(Shader_cache&&) = delete;
        ~Shader_cache() noexcept;

        [[nodiscard]] Gouraud_mrt_shader& gouraud() const;
        [[nodiscard]] Normals_shader& normals() const;
        [[nodiscard]] Plain_texture_shader& pts() const;
        [[nodiscard]] Colormapped_plain_texture_shader& colormapped_pts() const;
        [[nodiscard]] Edge_detection_shader& edge_detector() const;
        [[nodiscard]] Skip_msxaa_blitter_shader& skip_msxaa() const;
    };
}

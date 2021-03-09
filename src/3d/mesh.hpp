#pragma once

#include "src/3d/textured_vert.hpp"
#include "src/3d/untextured_vert.hpp"

#include <iterator>
#include <limits>
#include <utility>
#include <vector>

namespace osmv {
    template<typename TVert>
    struct Mesh {
        using element_index_type = unsigned short;
        using vertex_type = TVert;

        static constexpr element_index_type index_senteniel = static_cast<element_index_type>(-1);

        std::vector<TVert> vert_data;
        std::vector<element_index_type> indices;

        Mesh() = default;

        Mesh(std::vector<TVert> _vert_data, std::vector<element_index_type> _indices) :
            vert_data{std::move(_vert_data)},
            indices{std::move(_indices)} {
        }

        template<typename... Args>
        element_index_type vertex_emplace_back(Args&&... args) {
            size_t idx = vert_data.size();
            assert(idx <= std::numeric_limits<element_index_type>::max());
            vert_data.emplace_back(std::forward<Args>(args)...);

            return static_cast<element_index_type>(idx);
        }

        void push_vert_data_and_index(TVert v) {
            element_index_type idx = vertex_emplace_back(v);
            indices.push_back(idx);
        }

        void clear() {
            vert_data.clear();
            indices.clear();
        }
    };

    struct Plain_mesh final : public Mesh<Untextured_vert> {
        using Mesh::Mesh;

        // warning: could be expensive
        [[nodiscard]] static Plain_mesh by_deduping(std::vector<Untextured_vert>);

        [[nodiscard]] static Plain_mesh from_raw_verts(std::vector<Untextured_vert>);

        [[nodiscard]] static Plain_mesh from_raw_verts(Untextured_vert const* first, size_t n);

        template<typename Container>
        [[nodiscard]] static Plain_mesh from_raw_verts(Container const& c) {
            return Plain_mesh::from_raw_verts(c.data(), c.size());
        }
    };

    struct Textured_mesh final : public Mesh<Textured_vert> {
        using Mesh::Mesh;

        [[nodiscard]] static Textured_mesh from_raw_verts(std::vector<Textured_vert>);

        [[nodiscard]] static Textured_mesh from_raw_verts(Textured_vert const* first, size_t n);

        template<typename Container>
        [[nodiscard]] static Textured_mesh from_raw_verts(Container const& c) {
            return Textured_mesh::from_raw_verts(c.data(), c.size());
        }
    };
}

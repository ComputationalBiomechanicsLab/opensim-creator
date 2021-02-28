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

        void generate_trivial_indices() {
            size_t n = vert_data.size();
            indices.resize(n);
            for (size_t i = 0; i < n; ++i) {
                indices[i] = i;
            }
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
    };

    struct Textured_mesh final : public Mesh<Textured_vert> {
        using Mesh::Mesh;
    };
}

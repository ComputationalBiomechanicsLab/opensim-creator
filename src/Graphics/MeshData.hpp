#pragma once

#include "src/Graphics/Renderer.hpp"

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <vector>

namespace osc
{
    // CPU-side mesh
    //
    // These can be generated/manipulated on any CPU core without having to worry
    // about the GPU
    //
    // see `Mesh` for the GPU-facing and user-friendly version of this. This separation
    // exists because the algs in this header are supposed to be simple and portable,
    // so that lower-level CPU-only code can use these without having to worry
    // about which GPU API is active, buffer packing, etc.
    struct MeshData {
        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;
        std::vector<uint32_t> indices;
        experimental::MeshTopography topography = experimental::MeshTopography::Triangles;

        void clear();
        void reserve(size_t);
    };

    // prints top-level mesh information (eg amount of each thing) to the stream
    std::ostream& operator<<(std::ostream&, MeshData const&);
}

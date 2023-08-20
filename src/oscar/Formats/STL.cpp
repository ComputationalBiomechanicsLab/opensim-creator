#include "STL.hpp"

#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Maths/Triangle.hpp"
#include "oscar/Utils/Assertions.hpp"
#include "oscar/Utils/Cpp20Shims.hpp"
#include "OscarConfiguration.hpp"

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace
{
    constexpr std::string_view c_Header = "Exported from " OSC_APPNAME_STRING;

    template<typename T>
    T const& ElementAt(nonstd::span<T const> vs, ptrdiff_t i)
    {
        if (i <= ssize(vs))
        {
            return vs[i];
        }
        else
        {
            throw std::out_of_range{"invalid span subscript"};
        }
    }

    void WriteHeader(std::ostream& o)
    {
        static_assert(c_Header.size() < 80);

        o << c_Header;
        for (ptrdiff_t i = 0; i < 80-osc::ssize(c_Header); ++i)
        {
            o << static_cast<uint8_t>(0x00);
        }
    }

    void WriteLittleEndianU32(std::ostream& o, uint32_t v)
    {
        o << static_cast<uint8_t>(v & 0xff);
        o << static_cast<uint8_t>((v>>8) & 0xff);
        o << static_cast<uint8_t>((v>>16) & 0xff);
        o << static_cast<uint8_t>((v>>24) & 0xff);
    }

    void WriteNumTriangles(std::ostream& o, osc::Mesh const& mesh)
    {
        OSC_ASSERT(mesh.getIndices().size()/3 <= std::numeric_limits<uint32_t>::max());
        WriteLittleEndianU32(o, static_cast<uint32_t>(mesh.getIndices().size()/3));
    }

    void WriteFloatIEEE(std::ostream& o, float v)
    {
        static_assert(sizeof(float) == 4);
        static_assert(sizeof(uint8_t) == 1);

        auto const* const ptr = reinterpret_cast<uint8_t const*>(&v);
        o << ptr[0];
        o << ptr[1];
        o << ptr[2];
        o << ptr[3];
    }

    void WriteVec3IEEE(std::ostream& o, glm::vec3 const& v)
    {
        WriteFloatIEEE(o, v.x);
        WriteFloatIEEE(o, v.y);
        WriteFloatIEEE(o, v.z);
    }

    void WriteAttributeCount(std::ostream& o)
    {
        o << static_cast<uint8_t>(0x00);
        o << static_cast<uint8_t>(0x00);
    }

    void WriteTriangle(std::ostream& o, osc::Triangle const& triangle)
    {
        WriteVec3IEEE(o, glm::normalize(osc::TriangleNormal(triangle)));
        WriteVec3IEEE(o, triangle.p0);
        WriteVec3IEEE(o, triangle.p1);
        WriteVec3IEEE(o, triangle.p2);
        WriteAttributeCount(o);
    }

    void WriteTriangles(std::ostream& o, osc::Mesh const& mesh)
    {
        osc::MeshIndicesView const indices = mesh.getIndices();
        nonstd::span<glm::vec3 const> const verts = mesh.getVerts();

        for (ptrdiff_t i = 0; i < ssize(indices)-2; i += 3)
        {
            osc::Triangle const t
            {
                ElementAt(verts, indices[i]),
                ElementAt(verts, indices[i+1]),
                ElementAt(verts, indices[i+2]),
            };
            WriteTriangle(o, t);
        }
    }
}

void osc::WriteMeshAsStl(std::ostream& output, Mesh const& mesh)
{
    if (mesh.getTopology() != MeshTopology::Triangles)
    {
        return;
    }

    WriteHeader(output);
    WriteNumTriangles(output, mesh);
    WriteTriangles(output, mesh);
}

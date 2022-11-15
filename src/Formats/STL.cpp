#include "STL.hpp"

#include "src/Graphics/Mesh.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Maths/Triangle.hpp"
#include "src/Utils/Assertions.hpp"

#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

static constexpr std::string_view const c_Header = "Exported from OpenSim Creator";

namespace
{
    template<typename T>
    T const& ElementAt(nonstd::span<T const> vs, size_t i)
    {
        if (i <= vs.size())
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
        for (size_t i = 0; i < 80-c_Header.size(); ++i)
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

        uint8_t const* const ptr = reinterpret_cast<uint8_t const*>(&v);  // typecasting to a byte is always safe
        o << static_cast<uint8_t>(ptr[0]);
        o << static_cast<uint8_t>(ptr[1]);
        o << static_cast<uint8_t>(ptr[2]);
        o << static_cast<uint8_t>(ptr[3]);
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
        if (indices.size() < 2)
        {
            return;
        }

        nonstd::span<glm::vec3 const> const verts = mesh.getVerts();

        for (size_t i = 0; i < indices.size()-2; i += 3)
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

void osc::StlWriter::write(Mesh const& mesh)
{
    if (mesh.getTopography() != MeshTopography::Triangles)
    {
        return;
    }

    WriteHeader(m_OutputStream);
    WriteNumTriangles(m_OutputStream, mesh);
    WriteTriangles(m_OutputStream, mesh);
}
#include "STL.hpp"

#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Triangle.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Shims/Cpp20/bit.hpp>
#include <oscar/Utils/Assertions.hpp>
#include <oscar/Utils/SpanHelpers.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <limits>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    std::string CreateHeaderText(osc::StlMetadata const& metadata)
    {
        std::stringstream ss;
        ss << "created " << std::put_time(&metadata.creationTime, "%Y-%m-%d %H:%M:%S") << " by " << metadata.authoringTool;
        return std::move(ss).str();
    }

    void WriteHeader(std::ostream& o, osc::StlMetadata const& metadata)
    {
        constexpr size_t c_NumBytesInSTLHeader = 80;
        constexpr size_t c_MaxCharsInSTLHeader = c_NumBytesInSTLHeader - 1;  // nul-terminator

        std::string const content = CreateHeaderText(metadata);
        size_t const len = std::min(content.size(), c_MaxCharsInSTLHeader);

        for (size_t i = 0; i < len; ++i)
        {
            o << static_cast<uint8_t>(content[i]);
        }
        for (size_t i = 0; i < c_NumBytesInSTLHeader-len; ++i)
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
        static_assert(std::numeric_limits<float>::is_iec559);
        for (uint8_t byte : osc::bit_cast<std::array<uint8_t, sizeof(decltype(v))>>(v))
        {
            o << byte;
        }
    }

    void WriteVec3IEEE(std::ostream& o, osc::Vec3 const& v)
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
        WriteVec3IEEE(o, osc::Normalize(osc::TriangleNormal(triangle)));
        WriteVec3IEEE(o, triangle.p0);
        WriteVec3IEEE(o, triangle.p1);
        WriteVec3IEEE(o, triangle.p2);
        WriteAttributeCount(o);
    }

    void WriteTriangles(std::ostream& o, osc::Mesh const& mesh)
    {
        osc::MeshIndicesView const indices = mesh.getIndices();
        std::span<osc::Vec3 const> const verts = mesh.getVerts();

        for (ptrdiff_t i = 0; i < std::ssize(indices)-2; i += 3)
        {
            osc::Triangle const t
            {
                osc::At(verts, indices[i]),
                osc::At(verts, indices[i+1]),
                osc::At(verts, indices[i+2]),
            };
            WriteTriangle(o, t);
        }
    }
}

// public API

osc::StlMetadata::StlMetadata(
    std::string_view authoringTool_) :

    authoringTool{authoringTool_},
    creationTime{GetSystemCalendarTime()}
{
}

void osc::WriteMeshAsStl(
    std::ostream& output,
    Mesh const& mesh,
    StlMetadata const& metadata)
{
    if (mesh.getTopology() != MeshTopology::Triangles)
    {
        return;
    }

    WriteHeader(output, metadata);
    WriteNumTriangles(output, mesh);
    WriteTriangles(output, mesh);
}

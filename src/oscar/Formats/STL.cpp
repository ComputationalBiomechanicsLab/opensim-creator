#include "STL.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/TriangleFunctions.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/os.h>
#include <oscar/Shims/Cpp20/bit.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/ObjectRepresentation.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

using namespace osc;

namespace
{
    std::string calcHeaderText(const StlMetadata& metadata)
    {
        std::stringstream ss;
        ss << "created " << std::put_time(&metadata.creationTime, "%Y-%m-%d %H:%M:%S") << " by " << metadata.authoringTool;
        return std::move(ss).str();
    }

    void writeHeader(std::ostream& o, const StlMetadata& metadata)
    {
        constexpr size_t c_NumBytesInSTLHeader = 80;
        constexpr size_t c_MaxCharsInSTLHeader = c_NumBytesInSTLHeader - 1;  // nul-terminator

        const std::string content = calcHeaderText(metadata);
        const size_t len = min(content.size(), c_MaxCharsInSTLHeader);

        for (size_t i = 0; i < len; ++i) {
            o << static_cast<uint8_t>(content[i]);
        }
        for (size_t i = 0; i < c_NumBytesInSTLHeader-len; ++i) {
            o << static_cast<uint8_t>(0x00);
        }
    }

    void writeLittleEndianU32(std::ostream& o, uint32_t v)
    {
        o << static_cast<uint8_t>(v & 0xff);
        o << static_cast<uint8_t>((v>>8) & 0xff);
        o << static_cast<uint8_t>((v>>16) & 0xff);
        o << static_cast<uint8_t>((v>>24) & 0xff);
    }

    void writeNumTriangles(std::ostream& o, const Mesh& mesh)
    {
        OSC_ASSERT(mesh.getNumIndices()/3 <= std::numeric_limits<uint32_t>::max());
        writeLittleEndianU32(o, static_cast<uint32_t>(mesh.getNumIndices()/3));
    }

    void writeFloatIEEE(std::ostream& o, float v)
    {
        static_assert(std::numeric_limits<float>::is_iec559, "STL files use IEE754 floats");
        for (std::byte byte : ViewObjectRepresentation(v)) {
            o << static_cast<uint8_t>(byte);
        }
    }

    void writeVec3IEEE(std::ostream& o, const Vec3& v)
    {
        writeFloatIEEE(o, v.x);
        writeFloatIEEE(o, v.y);
        writeFloatIEEE(o, v.z);
    }

    void writeAttributeCount(std::ostream& o)
    {
        o << static_cast<uint8_t>(0x00);
        o << static_cast<uint8_t>(0x00);
    }

    void writeTriangle(std::ostream& o, const Triangle& triangle)
    {
        writeVec3IEEE(o, triangle_normal(triangle));
        writeVec3IEEE(o, triangle.p0);
        writeVec3IEEE(o, triangle.p1);
        writeVec3IEEE(o, triangle.p2);
        writeAttributeCount(o);
    }

    void writeTriangles(std::ostream& o, const Mesh& mesh)
    {
        mesh.forEachIndexedTriangle([&o](Triangle t) { writeTriangle(o, t); });
    }
}

// public API

osc::StlMetadata::StlMetadata(
    std::string_view authoringTool_) :

    authoringTool{authoringTool_},
    creationTime{GetSystemCalendarTime()}
{}

void osc::writeMeshAsStl(
    std::ostream& output,
    const Mesh& mesh,
    const StlMetadata& metadata)
{
    if (mesh.getTopology() != MeshTopology::Triangles) {
        return;
    }

    writeHeader(output, metadata);
    writeNumTriangles(output, mesh);
    writeTriangles(output, mesh);
}

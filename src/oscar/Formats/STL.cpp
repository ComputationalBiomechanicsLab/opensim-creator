#include "STL.h"

#include <oscar/Graphics/Mesh.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/Triangle.h>
#include <oscar/Maths/TriangleFunctions.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/os.h>
#include <oscar/Utils/Algorithms.h>
#include <oscar/Utils/Assertions.h>
#include <oscar/Utils/ObjectRepresentation.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <ostream>
#include <limits>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

using namespace osc;

namespace
{
    std::string calc_header_text(const StlMetadata& metadata)
    {
        std::stringstream ss;
        ss << "created " << std::put_time(&metadata.creation_time, "%Y-%m-%d %H:%M:%S") << " by " << metadata.authoring_tool;
        return std::move(ss).str();
    }

    void write_header(std::ostream& o, const StlMetadata& metadata)
    {
        constexpr size_t c_num_bytes_in_stl_header = 80;
        constexpr size_t c_max_chars_in_stl_header = c_num_bytes_in_stl_header - 1;  // nul-terminator

        const std::string content = calc_header_text(metadata);
        const size_t len = min(content.size(), c_max_chars_in_stl_header);

        for (size_t i = 0; i < len; ++i) {
            o << static_cast<uint8_t>(content[i]);
        }
        for (size_t i = 0; i < c_num_bytes_in_stl_header-len; ++i) {
            o << static_cast<uint8_t>(0x00);
        }
    }

    void write_u32_little_endian(std::ostream& o, uint32_t v)
    {
        o << static_cast<uint8_t>(v & 0xff);
        o << static_cast<uint8_t>((v>>8) & 0xff);
        o << static_cast<uint8_t>((v>>16) & 0xff);
        o << static_cast<uint8_t>((v>>24) & 0xff);
    }

    void write_num_triangles(std::ostream& o, const Mesh& mesh)
    {
        OSC_ASSERT(mesh.num_indices()/3 <= std::numeric_limits<uint32_t>::max());
        write_u32_little_endian(o, static_cast<uint32_t>(mesh.num_indices()/3));
    }

    void write_float_ieee754(std::ostream& o, float v)
    {
        static_assert(std::numeric_limits<float>::is_iec559, "STL files use IEE754 floats");
        for (std::byte byte : view_object_representation(v)) {
            o << static_cast<uint8_t>(byte);
        }
    }

    void write_vec3_ieee754(std::ostream& o, const Vec3& v)
    {
        write_float_ieee754(o, v.x);
        write_float_ieee754(o, v.y);
        write_float_ieee754(o, v.z);
    }

    void write_attribute_count(std::ostream& o)
    {
        o << static_cast<uint8_t>(0x00);
        o << static_cast<uint8_t>(0x00);
    }

    void write_triangle(std::ostream& o, const Triangle& triangle)
    {
        write_vec3_ieee754(o, triangle_normal(triangle));
        write_vec3_ieee754(o, triangle.p0);
        write_vec3_ieee754(o, triangle.p1);
        write_vec3_ieee754(o, triangle.p2);
        write_attribute_count(o);
    }

    void write_triangles(std::ostream& o, const Mesh& mesh)
    {
        mesh.for_each_indexed_triangle([&o](Triangle t) { write_triangle(o, t); });
    }
}


osc::StlMetadata::StlMetadata(
    std::string_view authoring_tool_) :

    authoring_tool{authoring_tool_},
    creation_time{system_calendar_time()}
{}

void osc::write_as_stl(
    std::ostream& output,
    const Mesh& mesh,
    const StlMetadata& metadata)
{
    if (mesh.topology() != MeshTopology::Triangles) {
        return;
    }

    write_header(output, metadata);
    write_num_triangles(output, mesh);
    write_triangles(output, mesh);
}

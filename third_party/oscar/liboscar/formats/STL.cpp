#include "STL.h"

#include <liboscar/graphics/Mesh.h>
#include <liboscar/maths/MathHelpers.h>
#include <liboscar/maths/Triangle.h>
#include <liboscar/maths/TriangleFunctions.h>
#include <liboscar/maths/Vector3.h>
#include <liboscar/platform/os.h>
#include <liboscar/platform/Strings.h>
#include <liboscar/utils/Algorithms.h>
#include <liboscar/utils/Assertions.h>
#include <liboscar/utils/ObjectRepresentation.h>

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
    std::string calc_header_text(const STLMetadata& metadata)
    {
        std::stringstream ss;
        ss << "created " << std::put_time(&metadata.creation_time, "%Y-%m-%d %H:%M:%S") << " by " << metadata.authoring_tool;
        return std::move(ss).str();
    }

    void write_header(std::ostream& out, const STLMetadata& metadata)
    {
        constexpr size_t c_num_bytes_in_stl_header = 80;
        constexpr size_t c_max_chars_in_stl_header = c_num_bytes_in_stl_header - 1;  // nul-terminator

        const std::string header_content = calc_header_text(metadata);
        const size_t len = min(header_content.size(), c_max_chars_in_stl_header);

        for (size_t i = 0; i < len; ++i) {
            out << static_cast<uint8_t>(header_content[i]);
        }
        for (size_t i = 0; i < c_num_bytes_in_stl_header-len; ++i) {
            out << static_cast<uint8_t>(0x00);
        }
    }

    void write_u32_little_endian(std::ostream& out, uint32_t v)
    {
        out << static_cast<uint8_t>((v    ) & 0xff);
        out << static_cast<uint8_t>((v>>8 ) & 0xff);
        out << static_cast<uint8_t>((v>>16) & 0xff);
        out << static_cast<uint8_t>((v>>24) & 0xff);
    }

    void write_num_triangles(std::ostream& out, const Mesh& mesh)
    {
        OSC_ASSERT(mesh.num_indices()/3 <= std::numeric_limits<uint32_t>::max());
        write_u32_little_endian(out, static_cast<uint32_t>(mesh.num_indices()/3));
    }

    void write_float_ieee754(std::ostream& out, float v)
    {
        static_assert(std::numeric_limits<float>::is_iec559, "STL files use IEE754 floats");
        for (const std::byte& byte : view_object_representation(v)) {
            out << static_cast<uint8_t>(byte);
        }
    }

    void write_vector3_ieee754(std::ostream& out, const Vector3& v)
    {
        write_float_ieee754(out, v.x);
        write_float_ieee754(out, v.y);
        write_float_ieee754(out, v.z);
    }

    void write_attribute_count(std::ostream& out)
    {
        out << static_cast<uint8_t>(0x00);
        out << static_cast<uint8_t>(0x00);
    }

    void write_triangle(std::ostream& out, const Triangle& triangle)
    {
        write_vector3_ieee754(out, triangle_normal(triangle));
        write_vector3_ieee754(out, triangle.p0);
        write_vector3_ieee754(out, triangle.p1);
        write_vector3_ieee754(out, triangle.p2);
        write_attribute_count(out);
    }

    void write_triangles(std::ostream& out, const Mesh& mesh)
    {
        mesh.for_each_indexed_triangle([&out](Triangle triangle) { write_triangle(out, triangle); });
    }
}

osc::STLMetadata::STLMetadata() :
    STLMetadata{strings::library_name()}
{}

osc::STLMetadata::STLMetadata(
    std::string_view authoring_tool_) :

    authoring_tool{authoring_tool_},
    creation_time{system_calendar_time()}
{}

void osc::STL::write(
    std::ostream& output,
    const Mesh& mesh,
    const STLMetadata& metadata)
{
    if (mesh.topology() != MeshTopology::Triangles) {
        return;
    }

    write_header(output, metadata);
    write_num_triangles(output, mesh);
    write_triangles(output, mesh);
}

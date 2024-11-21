#include "DAE.h"

#include <oscar/Graphics/Color.h>
#include <oscar/Graphics/Mesh.h>
#include <oscar/Graphics/Scene/SceneDecoration.h>
#include <oscar/Maths/Mat4.h>
#include <oscar/Maths/MathHelpers.h>
#include <oscar/Maths/VecFunctions.h>
#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/Platform/os.h>
#include <oscar/Strings.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <iomanip>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace osc;

namespace
{
    struct DaeGeometry final {

        DaeGeometry(std::string geometry_id_, Mesh mesh_) :
            geometry_id{std::move(geometry_id_)},
            mesh{std::move(mesh_)}
        {}

        std::string geometry_id;
        Mesh mesh;
    };

    struct DaeMaterial final {

        DaeMaterial(
            std::string material_id_,
            const Color& color_) :

            material_id{std::move(material_id_)},
            color{color_}
        {}

        std::string material_id;
        Color color;
    };

    struct DaeInstance final {

        DaeInstance(
            std::string instance_id_,
            std::string geometry_id_,
            std::string material_id_,
            const Transform& transform_) :

            instance_id{std::move(instance_id_)},
            geometry_id{std::move(geometry_id_)},
            material_id{std::move(material_id_)},
            transform{transform_}
        {}

        std::string instance_id;
        std::string geometry_id;
        std::string material_id;
        Transform transform;
    };

    // internal representation of a data structure that more closely resembles
    // how DAE files are structured
    struct DaeSceneGraph final {
        std::vector<DaeGeometry> geometries;
        std::vector<DaeMaterial> materials;
        std::vector<DaeInstance> instances;
    };

    DaeSceneGraph to_dae_scene_graph(std::span<const SceneDecoration> decorations)
    {
        DaeSceneGraph rv;

        std::unordered_map<Mesh, std::string> mesh_to_id;
        mesh_to_id.reserve(decorations.size());  // upper limit
        std::unordered_map<Color, std::string> color_to_material_id;
        color_to_material_id.reserve(decorations.size());  // upper limit
        size_t latest_mesh = 0;
        size_t latest_material = 0;
        size_t latest_instance = 0;

        for (const SceneDecoration& decoration : decorations) {

            if (decoration.mesh.topology() != MeshTopology::Triangles) {
                continue;  // unsupported
            }

            if (not std::holds_alternative<Color>(decoration.shading)) {
                continue;  // custom materials are unsupported
            }

            auto [mesh_iterator, mesh_inserted] = mesh_to_id.try_emplace(decoration.mesh, std::string{});
            if (mesh_inserted) {
                std::stringstream id;
                id << "mesh_" << latest_mesh++;
                mesh_iterator->second = std::move(id).str();

                rv.geometries.emplace_back(mesh_iterator->second, mesh_iterator->first);
            }

            auto [material_iterator, material_inserted] = color_to_material_id.try_emplace(std::get<Color>(decoration.shading), std::string{});
            if (material_inserted) {
                std::stringstream id;
                id << "material_" << latest_material++;
                material_iterator->second = std::move(id).str();

                rv.materials.emplace_back(material_iterator->second, material_iterator->first);
            }

            std::stringstream instance_id;
            instance_id << "instance_" << latest_instance++;
            rv.instances.emplace_back(std::move(instance_id).str(), mesh_iterator->second, material_iterator->second, decoration.transform);
        }

        return rv;
    }

    std::span<const float> to_float_span(std::span<const Vec2> vec2_span)
    {
        return {value_ptr(vec2_span[0]), 2 * vec2_span.size()};
    }

    std::span<const float> to_float_span(std::span<const Vec3> vec3_span)
    {
        return {value_ptr(vec3_span[0]), 3 * vec3_span.size()};
    }

    std::span<const float> to_float_span(const Color& v)
    {
        return {value_ptr(v), 4};
    }

    std::ostream& operator<<(std::ostream& out, std::span<const float> values)
    {
        std::string_view delimiter;
        std::array<char, 512> buffer{};
        for (const float value : values) {
            if (const auto size = std::snprintf(buffer.data(), buffer.size(), "%f", value); size > 0) {
                out << delimiter << std::string_view{buffer.data(), static_cast<size_t>(size)};
                delimiter = " ";
            }
        }
        return out;
    }

    void write_header(std::ostream& out)
    {
        out << R"(<?xml version="1.0" encoding="utf-8"?>)";
        out << '\n';
    }

    void write_collada_root_node_begin(std::ostream& out)
    {
        out << R"(<COLLADA xmlns = "http://www.collada.org/2005/11/COLLADASchema" version = "1.4.1" xmlns:xsi = "http://www.w3.org/2001/XMLSchema-instance">)";
        out << '\n';
    }

    void write_collada_root_node_end(std::ostream& out)
    {
        out << R"(</COLLADA>)";
        out << '\n';
    }

    void write_top_level_asset_node(std::ostream& out, const DAEMetadata& metadata)
    {
        out << "  <asset>\n";
        out << "    <contributor>\n";
        out << "      <author>" << metadata.author << "</author>\n";
        out << "      <authoring_tool>" << metadata.authoring_tool << "</authoring_tool>\n";
        out << "    </contributor>\n";
        out << "    <created>" << std::put_time(&metadata.creation_time, "%Y-%m-%d %H:%M:%S") << "</created>\n";
        out << "    <modified>" << std::put_time(&metadata.modification_time, "%Y-%m-%d %H:%M:%S") << "</modified>\n";
        out << "    <unit name=\"meter\" meter=\"1\" />\n";
        out << "    <up_axis>Y_UP</up_axis>\n";
        out << "  </asset>\n";
    }

    void write_effect_node(std::ostream& out, const DaeMaterial& material)
    {
        out << "    <effect id=\"" << material.material_id << "-effect\">\n";
        out << "      <profile_COMMON>\n";
        out << "        <technique sid=\"common\">\n";
        out << "          <lambert>\n";
        out << "            <emission>\n";
        out << "              <color sid=\"emission\">0 0 0 1</color>\n";
        out << "            </emission>\n";
        out << "            <diffuse>\n";
        out << "              <color sid=\"diffuse\">" << to_float_span(material.color) << "</color>\n";
        out << "            </diffuse>\n";
        out << "            <reflectivity>\n";
        out << "              <float sid=\"specular\">0.0</float>\n";
        out << "            </reflectivity>\n";
        out << "          </lambert>\n";
        out << "        </technique>\n";
        out << "      </profile_COMMON>\n";
        out << "    </effect>\n";
    }

    void write_library_effects_node(std::ostream& out, std::span<const DaeMaterial> materials)
    {
        out << "  <library_effects>\n";
        for (const DaeMaterial& material : materials) {
            write_effect_node(out, material);
        }
        out << "  </library_effects>\n";
    }

    void write_material_node(std::ostream& out, const DaeMaterial& material)
    {
        out << "    <material id=\"" << material.material_id << "-material\" name=\"" << material.material_id << "\">\n";
        out << "      <instance_effect url=\"#" << material.material_id << "-effect\"/>\n";
        out << "    </material>\n";
    }

    void write_library_materials_node(std::ostream& out, std::span<const DaeMaterial> materials)
    {
        out << "  <library_materials>\n";
        for (const DaeMaterial& material : materials) {
            write_material_node(out, material);
        }
        out << "  </library_materials>\n";
    }

    void write_mesh_positions_source_node(std::ostream& out, const DaeGeometry& geometry)
    {
        const auto vertices = geometry.mesh.vertices();
        const size_t num_floats = 3 * vertices.size();
        const size_t num_vertices = vertices.size();

        out << "        <source id=\"" << geometry.geometry_id << "-positions\">\n";
        out << "          <float_array id=\"" << geometry.geometry_id << "-positions-array\" count=\"" << num_floats << "\">" << to_float_span(vertices) << "</float_array>\n";
        out << "          <technique_common>\n";
        out << "            <accessor source=\"#" << geometry.geometry_id << "-positions-array\" count=\"" << num_vertices << "\" stride=\"3\">\n";
        out << "              <param name=\"X\" type=\"float\"/>\n";
        out << "              <param name=\"Y\" type=\"float\"/>\n";
        out << "              <param name=\"Z\" type=\"float\"/>\n";
        out << "            </accessor>\n";
        out << "          </technique_common>\n";
        out << "        </source>\n";
    }

    void write_mesh_normals_source_node(std::ostream& out, const DaeGeometry& geometry)
    {
        const auto normals = geometry.mesh.normals();
        const size_t num_floats = 3 * normals.size();
        const size_t num_normals = normals.size();

        out << "        <source id=\""  << geometry.geometry_id << "-normals\">\n";
        out << "          <float_array id=\"" << geometry.geometry_id << "-normals-array\" count=\"" << num_floats << "\">" << to_float_span(normals) << "</float_array>\n";
        out << "          <technique_common>\n";
        out << "            <accessor source=\"#" << geometry.geometry_id << "-normals-array\" count=\"" << num_normals << "\" stride=\"3\">\n";
        out << "              <param name=\"X\" type=\"float\"/>\n";
        out << "              <param name=\"Y\" type=\"float\"/>\n";
        out << "              <param name=\"Z\" type=\"float\"/>\n";
        out << "            </accessor>\n";
        out << "          </technique_common>\n";
        out << "        </source>\n";
    }

    void write_mesh_texture_coordinates_source_node(std::ostream& out, const DaeGeometry& geometry)
    {
        const auto tex_coords = geometry.mesh.tex_coords();
        const size_t num_floats = 2 * tex_coords.size();
        const size_t num_tex_coords = tex_coords.size();

        out << "        <source id=\"" << geometry.geometry_id << "-map-0\">\n";
        out << "          <float_array id=\"" << geometry.geometry_id << "-map-0-array\" count=\"" << num_floats << "\">" << to_float_span(tex_coords) <<  "</float_array>\n";
        out << "          <technique_common>\n";
        out << "            <accessor source=\"#" << geometry.geometry_id << "-map-0-array\" count=\"" << num_tex_coords << "\" stride=\"2\">\n";
        out << "              <param name=\"S\" type=\"float\"/>\n";
        out << "              <param name=\"T\" type=\"float\"/>\n";
        out << "            </accessor>\n";
        out << "          </technique_common>\n";
        out << "        </source>\n";
    }

    void write_mesh_vertices_node(std::ostream& out, const DaeGeometry& geometry)
    {
        out << "        <vertices id=\"" << geometry.geometry_id << "-vertices\">\n";
        out << R"(           <input semantic="POSITION" source="#)" << geometry.geometry_id << "-positions\"/>\n";
        out << "        </vertices>\n";
    }

    void write_mesh_triangles_node(std::ostream& out, const DaeGeometry& geometry)
    {
        const auto indices = geometry.mesh.indices();
        const size_t num_triangles = indices.size() / 3;

        out << "        <triangles count=\"" << num_triangles << "\">\n";
        out << R"(            <input semantic="VERTEX" source="#)" << geometry.geometry_id << "-vertices\" offset=\"0\" />\n";
        if (geometry.mesh.has_normals()) {
            out << R"(            <input semantic="NORMAL" source="#)" << geometry.geometry_id << "-normals\" offset=\"0\" />\n";
        }
        if (geometry.mesh.has_tex_coords()) {
            out << R"(            <input semantic="TEXCOORD" source="#)" << geometry.geometry_id << "-map-0\" offset=\"0\" set=\"0\"/>\n";
        }

        out << "          <p>";
        std::string_view delimiter;
        for (const auto index : indices) {
            out << delimiter << index;
            delimiter = " ";
        }
        out << "</p>\n";
        out << "        </triangles>\n";
    }

    void write_mesh_node(std::ostream& out, const DaeGeometry& geometry)
    {
        out << R"(      <mesh>)";
        out << '\n';

        write_mesh_positions_source_node(out, geometry);
        if (geometry.mesh.has_normals()) {
            write_mesh_normals_source_node(out, geometry);
        }
        if (geometry.mesh.has_tex_coords()) {
            write_mesh_texture_coordinates_source_node(out, geometry);
        }
        write_mesh_vertices_node(out, geometry);
        write_mesh_triangles_node(out, geometry);

        out << R"(      </mesh>)";
        out << '\n';
    }

    void write_geometry_node(std::ostream& out, const DaeGeometry& geometry)
    {
        out << "    <geometry id=\"" << geometry.geometry_id << "\" name=\"" << geometry.geometry_id << "\">\n";
        write_mesh_node(out, geometry);
        out << "    </geometry>\n";
    }

    void write_library_geometries_node(std::ostream& out, std::span<const DaeGeometry> geometries)
    {
        out << "  <library_geometries>\n";
        for (const DaeGeometry& geometry : geometries) {
            write_geometry_node(out, geometry);
        }
        out << "  </library_geometries>\n";
    }

    void write_matrix_node(std::ostream& out, const Transform& transform)
    {
        const Mat4 m = mat4_cast(transform);

        // row-major
        out << R"(        <matrix sid="transform">)";
        std::string_view delimiter;
        for (Mat4::size_type row = 0; row < 4; ++row) {
            out << delimiter << m[0][row];
            delimiter = " ";
            out << delimiter << m[1][row];
            out << delimiter << m[2][row];
            out << delimiter << m[3][row];
        }
        out << R"(</matrix>)";
        out << '\n';
    }

    void write_instance_bind_material_node(std::ostream& out, const DaeInstance& instance)
    {
        out << "          <bind_material>\n";
        out << "            <technique_common>\n";
        out << "              <instance_material symbol=\"" << instance.material_id << "-material\" target=\"#" << instance.material_id << "-material\" />\n";
        out << "            </technique_common>\n";
        out << "          </bind_material>\n";
    }

    void write_instance_geometry_node(std::ostream& out, const DaeInstance& instance)
    {
        out << "        <instance_geometry url=\"#" << instance.geometry_id << "\" name=\"" << instance.geometry_id << "\">\n";
        write_instance_bind_material_node(out, instance);
        out << "        </instance_geometry>\n";
    }

    void write_scene_node(std::ostream& out, const DaeInstance& instance)
    {
        out << "      <node id=\"" << instance.instance_id << "\" name=\"" << instance.instance_id << "\" type=\"NODE\">\n";
        write_matrix_node(out, instance.transform);
        write_instance_geometry_node(out, instance);
        out << "      </node>\n";
    }

    void write_library_visual_scenes_node(std::ostream& out, const DaeSceneGraph& scene_graph)
    {
        out << R"(  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">)";
        out << '\n';

        for (const DaeInstance& instance : scene_graph.instances) {
            write_scene_node(out, instance);
        }

        out << R"(    </visual_scene>
  </library_visual_scenes>)";
        out << '\n';
    }
}

osc::DAEMetadata::DAEMetadata() :
    DAEMetadata{"unknown_author", strings::library_name()}
{}

osc::DAEMetadata::DAEMetadata(
    std::string_view author_,
    std::string_view authoring_tool_) :

    author{author_},
    authoring_tool{authoring_tool_},
    creation_time{system_calendar_time()},
    modification_time{creation_time}
{}

void osc::write_as_dae(
    std::ostream& out,
    std::span<const SceneDecoration> decorations,
    const DAEMetadata& metadata)
{
    const DaeSceneGraph graph = to_dae_scene_graph(decorations);

    write_header(out);
    write_collada_root_node_begin(out);
    write_top_level_asset_node(out, metadata);
    write_library_effects_node(out, graph.materials);
    write_library_materials_node(out, graph.materials);
    write_library_geometries_node(out, graph.geometries);
    write_library_visual_scenes_node(out, graph);
    write_collada_root_node_end(out);
}

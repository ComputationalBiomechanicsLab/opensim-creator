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

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace osc;

namespace
{
    struct DAEGeometry final {

        DAEGeometry(std::string geometry_id_, Mesh mesh_) :
            geometry_id{std::move(geometry_id_)},
            mesh{std::move(mesh_)}
        {}

        std::string geometry_id;
        Mesh mesh;
    };

    struct DAEMaterial final {

        DAEMaterial(
            std::string material_id_,
            const Color& color_) :

            material_id{std::move(material_id_)},
            color{color_}
        {}

        std::string material_id;
        Color color;
    };

    struct DAEInstance final {

        DAEInstance(
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

    // internal representation of a datastructure that more closely resembles
    // how DAE files are structured
    struct DAESceneGraph final {
        std::vector<DAEGeometry> geometries;
        std::vector<DAEMaterial> materials;
        std::vector<DAEInstance> instances;
    };

    DAESceneGraph to_dae_scene_graph(std::span<const SceneDecoration> decorations)
    {
        DAESceneGraph rv;

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

            auto [mesh_iter, mesh_inserted] = mesh_to_id.try_emplace(decoration.mesh, std::string{});
            if (mesh_inserted) {
                std::stringstream id;
                id << "mesh_" << latest_mesh++;
                mesh_iter->second = std::move(id).str();

                rv.geometries.emplace_back(mesh_iter->second, mesh_iter->first);
            }

            auto [material_iter, material_inserted] = color_to_material_id.try_emplace(std::get<Color>(decoration.shading), std::string{});
            if (material_inserted) {
                std::stringstream id;
                id << "material_" << latest_material++;
                material_iter->second = std::move(id).str();

                rv.materials.emplace_back(material_iter->second, material_iter->first);
            }

            std::stringstream instance_id;
            instance_id << "instance_" << latest_instance++;
            rv.instances.emplace_back(std::move(instance_id).str(), mesh_iter->second, material_iter->second, decoration.transform);
        }

        return rv;
    }

    std::span<const float> to_float_span(std::span<const Vec2> s)
    {
        return {value_ptr(s[0]), 2 * s.size()};
    }

    std::span<const float> to_float_span(std::span<const Vec3> s)
    {
        return {value_ptr(s[0]), 3 * s.size()};
    }

    std::span<const float> to_float_span(const Color& v)
    {
        return {value_ptr(v), 4};
    }

    std::ostream& operator<<(std::ostream& out, std::span<const float> values)
    {
        std::string_view delimeter;
        for (float value : values) {
            // note: to_string measures faster than directly streaming the value, because
            //       there's overhead associated with the stream that we don't care about
            //       for the DAE format (locale, etc.)
            out << delimeter << std::to_string(value);
            delimeter = " ";
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

    void write_effect_node(std::ostream& out, const DAEMaterial& material)
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

    void write_library_effects_node(std::ostream& out, std::span<const DAEMaterial> materials)
    {
        out << "  <library_effects>\n";
        for (const DAEMaterial& material : materials) {
            write_effect_node(out, material);
        }
        out << "  </library_effects>\n";
    }

    void write_material_node(std::ostream& out, const DAEMaterial& material)
    {
        out << "    <material id=\"" << material.material_id << "-material\" name=\"" << material.material_id << "\">\n";
        out << "      <instance_effect url=\"#" << material.material_id << "-effect\"/>\n";
        out << "    </material>\n";
    }

    void write_library_materials_node(std::ostream& out, std::span<const DAEMaterial> materials)
    {
        out << "  <library_materials>\n";
        for (const DAEMaterial& material : materials) {
            write_material_node(out, material);
        }
        out << "  </library_materials>\n";
    }

    void write_mesh_positions_source_node(std::ostream& out, const DAEGeometry& geometry)
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

    void write_mesh_normals_source_node(std::ostream& out, const DAEGeometry& geometry)
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

    void write_mesh_texture_coordinates_source_node(std::ostream& out, const DAEGeometry& geometry)
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

    void write_mesh_vertices_node(std::ostream& out, const DAEGeometry& geometry)
    {
        out << "        <vertices id=\"" << geometry.geometry_id << "-vertices\">\n";
        out << R"(           <input semantic="POSITION" source="#)" << geometry.geometry_id << "-positions\"/>\n";
        out << "        </vertices>\n";
    }

    void write_mesh_triangles_node(std::ostream& out, const DAEGeometry& geometry)
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
        std::string_view delimeter;
        for (auto index : indices) {
            out << delimeter << index;
            delimeter = " ";
        }
        out << "</p>\n";
        out << "        </triangles>\n";
    }

    void write_mesh_node(std::ostream& out, const DAEGeometry& geometry)
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

    void write_geometry_node(std::ostream& out, const DAEGeometry& geometry)
    {
        out << "    <geometry id=\"" << geometry.geometry_id << "\" name=\"" << geometry.geometry_id << "\">\n";
        write_mesh_node(out, geometry);
        out << "    </geometry>\n";
    }

    void write_library_geometries_node(std::ostream& out, std::span<const DAEGeometry> geometries)
    {
        out << "  <library_geometries>\n";
        for (const DAEGeometry& geometry : geometries) {
            write_geometry_node(out, geometry);
        }
        out << "  </library_geometries>\n";
    }

    void write_matrix_node(std::ostream& out, const Transform& transform)
    {
        const Mat4 m = mat4_cast(transform);

        // row-major
        out << R"(        <matrix sid="transform">)";
        std::string_view delimeter;
        for (Mat4::size_type row = 0; row < 4; ++row) {
            out << delimeter << m[0][row];
            delimeter = " ";
            out << delimeter << m[1][row];
            out << delimeter << m[2][row];
            out << delimeter << m[3][row];
        }
        out << R"(</matrix>)";
        out << '\n';
    }

    void write_instance_bind_material_node(std::ostream& out, const DAEInstance& instance)
    {
        out << "          <bind_material>\n";
        out << "            <technique_common>\n";
        out << "              <instance_material symbol=\"" << instance.material_id << "-material\" target=\"#" << instance.material_id << "-material\" />\n";
        out << "            </technique_common>\n";
        out << "          </bind_material>\n";
    }

    void write_instance_geometry_node(std::ostream& out, const DAEInstance& instance)
    {
        out << "        <instance_geometry url=\"#" << instance.geometry_id << "\" name=\"" << instance.geometry_id << "\">\n";
        write_instance_bind_material_node(out, instance);
        out << "        </instance_geometry>\n";
    }

    void write_scene_node(std::ostream& out, const DAEInstance& instance)
    {
        out << "      <node id=\"" << instance.instance_id << "\" name=\"" << instance.instance_id << "\" type=\"NODE\">\n";
        write_matrix_node(out, instance.transform);
        write_instance_geometry_node(out, instance);
        out << "      </node>\n";
    }

    void write_library_visual_scenes_node(std::ostream& out, const DAESceneGraph& scene_graph)
    {
        out << R"(  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">)";
        out << '\n';

        for (const DAEInstance& instance : scene_graph.instances) {
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
    const DAESceneGraph graph = to_dae_scene_graph(decorations);

    write_header(out);
    write_collada_root_node_begin(out);
    write_top_level_asset_node(out, metadata);
    write_library_effects_node(out, graph.materials);
    write_library_materials_node(out, graph.materials);
    write_library_geometries_node(out, graph.geometries);
    write_library_visual_scenes_node(out, graph);
    write_collada_root_node_end(out);
}

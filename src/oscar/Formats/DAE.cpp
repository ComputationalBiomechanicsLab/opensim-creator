#include "DAE.hpp"

#include <oscar/Graphics/Color.hpp>
#include <oscar/Graphics/Mesh.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/os.hpp>
#include <oscar/Scene/SceneDecoration.hpp>

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

using osc::Color;
using osc::DAEMetadata;
using osc::Mat4;
using osc::Mesh;
using osc::MeshTopology;
using osc::SceneDecoration;
using osc::Transform;
using osc::ValuePtr;
using osc::Vec2;
using osc::Vec3;

// scene-to-graph conversion stuff
namespace
{
    struct DAEGeometry final {

        DAEGeometry(std::string geometryID_, Mesh mesh_) :
            geometryID{std::move(geometryID_)},
            mesh{std::move(mesh_)}
        {
        }

        std::string geometryID;
        Mesh mesh;
    };

    struct DAEMaterial final {

        DAEMaterial(
            std::string materialID_,
            Color const& color_) :

            materialID{std::move(materialID_)},
            color{color_}
        {
        }

        std::string materialID;
        Color color;
    };

    struct DAEInstance final {

        DAEInstance(
            std::string instanceID_,
            std::string geometryID_,
            std::string materialID_,
            Transform const& transform_) :

            instanceID{std::move(instanceID_)},
            geometryID{std::move(geometryID_)},
            materialID{std::move(materialID_)},
            transform{transform_}
        {
        }

        std::string instanceID;
        std::string geometryID;
        std::string materialID;
        Transform transform;
    };

    // internal representation of a datastructure that more closely resembles
    // how DAE files are structured
    struct DAESceneGraph final {
        std::vector<DAEGeometry> geometries;
        std::vector<DAEMaterial> materials;
        std::vector<DAEInstance> instances;
    };

    DAESceneGraph ToDAESceneGraph(std::span<SceneDecoration const> els)
    {
        DAESceneGraph rv;

        int64_t latestMesh = 0;
        int64_t latestMaterial = 0;
        std::unordered_map<Mesh, std::string> mesh2id;
        std::unordered_map<Color, std::string> color2materialid;
        int64_t latestInstance = 0;

        for (SceneDecoration const& el : els)
        {
            if (el.mesh.getTopology() != MeshTopology::Triangles)
            {
                continue;  // unsupported
            }

            auto [meshIt, meshInserted] = mesh2id.try_emplace(el.mesh, std::string{});
            if (meshInserted)
            {
                std::stringstream id;
                id << "mesh_" << latestMesh++;
                meshIt->second = std::move(id).str();

                rv.geometries.emplace_back(meshIt->second, meshIt->first);
            }

            auto [materialIt, materialInserted] = color2materialid.try_emplace(el.color, std::string{});
            if (materialInserted)
            {
                std::stringstream id;
                id << "material_" << latestMaterial++;
                materialIt->second = std::move(id).str();

                rv.materials.emplace_back(materialIt->second, materialIt->first);
            }

            std::stringstream instanceID;
            instanceID << "instance_" << latestInstance++;
            rv.instances.emplace_back(std::move(instanceID).str(), meshIt->second, materialIt->second, el.transform);
        }

        return rv;
    }
}

// graph-writing stuff
namespace
{
    std::span<float const> ToFloatSpan(std::span<Vec2 const> s)
    {
        return {ValuePtr(s[0]), 2 * s.size()};
    }

    std::span<float const> ToFloatSpan(std::span<Vec3 const> s)
    {
        return {ValuePtr(s[0]), 3 * s.size()};
    }

    std::span<float const> ToFloatSpan(Color const& v)
    {
        return {ValuePtr(v), 4};
    }

    std::ostream& operator<<(std::ostream& out, std::span<float const> vs)
    {
        std::string_view delim;
        for (float v : vs)
        {
            // note: to_string measures faster than directly streaming the value, because
            //       there's overhead associated with the stream that we don't care about
            //       for the DAE format (locale, etc.)
            out << delim << std::to_string(v);
            delim = " ";
        }
        return out;
    }

    void WriteXMLHeader(std::ostream& o)
    {
        o << R"(<?xml version="1.0" encoding="utf-8"?>)";
        o << '\n';
    }

    void WriteCOLLADARootNodeBEGIN(std::ostream& o)
    {
        o << R"(<COLLADA xmlns = "http://www.collada.org/2005/11/COLLADASchema" version = "1.4.1" xmlns:xsi = "http://www.w3.org/2001/XMLSchema-instance">)";
        o << '\n';
    }

    void WriteCOLLADARootNodeEND(std::ostream& o)
    {
        o << R"(</COLLADA>)";
        o << '\n';
    }

    void WriteTopLevelAssetBlock(std::ostream& o, DAEMetadata const& metadata)
    {
        o << "  <asset>\n";
        o << "    <contributor>\n";
        o << "      <author>" << metadata.author << "</author>\n";
        o << "      <authoring_tool>" << metadata.authoringTool << "</authoring_tool>\n";
        o << "    </contributor>\n";
        o << "    <created>" << std::put_time(&metadata.creationTime, "%Y-%m-%d %H:%M:%S") << "</created>\n";
        o << "    <modified>" << std::put_time(&metadata.modificationTime, "%Y-%m-%d %H:%M:%S") << "</modified>\n";
        o << "    <unit name=\"meter\" meter=\"1\" />\n";
        o << "    <up_axis>Y_UP</up_axis>\n";
        o << "  </asset>\n";
    }

    void WriteEffect(std::ostream& o, DAEMaterial const& material)
    {
        o << "    <effect id=\"" << material.materialID << "-effect\">\n";
        o << "      <profile_COMMON>\n";
        o << "        <technique sid=\"common\">\n";
        o << "          <lambert>\n";
        o << "            <emission>\n";
        o << "              <color sid=\"emission\">0 0 0 1</color>\n";
        o << "            </emission>\n";
        o << "            <diffuse>\n";
        o << "              <color sid=\"diffuse\">" << ToFloatSpan(material.color) << "</color>\n";
        o << "            </diffuse>\n";
        o << "            <reflectivity>\n";
        o << "              <float sid=\"specular\">0.0</float>\n";
        o << "            </reflectivity>\n";
        o << "          </lambert>\n";
        o << "        </technique>\n";
        o << "      </profile_COMMON>\n";
        o << "    </effect>\n";
    }

    void WriteLibraryEffects(std::ostream& o, std::span<DAEMaterial const> materials)
    {
        o << "  <library_effects>\n";
        for (DAEMaterial const& material : materials)
        {
            WriteEffect(o, material);
        }
        o << "  </library_effects>\n";
    }

    void WriteMaterial(std::ostream& o, DAEMaterial const& material)
    {
        o << "    <material id=\"" << material.materialID << "-material\" name=\"" << material.materialID << "\">\n";
        o << "      <instance_effect url=\"#" << material.materialID << "-effect\"/>\n";
        o << "    </material>\n";
    }

    void WriteLibraryMaterials(std::ostream& o, std::span<DAEMaterial const> materials)
    {
        o << "  <library_materials>\n";
        for (DAEMaterial const& material : materials)
        {
            WriteMaterial(o, material);
        }
        o << "  </library_materials>\n";
    }

    void WriteMeshPositionsSource(std::ostream& o, DAEGeometry const& geom)
    {
        auto const vals = geom.mesh.getVerts();
        size_t const floatCount = 3 * vals.size();
        size_t const vertCount = vals.size();

        o << "        <source id=\"" << geom.geometryID << "-positions\">\n";
        o << "          <float_array id=\"" << geom.geometryID << "-positions-array\" count=\"" << floatCount << "\">" << ToFloatSpan(vals) << "</float_array>\n";
        o << "          <technique_common>\n";
        o << "            <accessor source=\"#" << geom.geometryID << "-positions-array\" count=\"" << vertCount << "\" stride=\"3\">\n";
        o << "              <param name=\"X\" type=\"float\"/>\n";
        o << "              <param name=\"Y\" type=\"float\"/>\n";
        o << "              <param name=\"Z\" type=\"float\"/>\n";
        o << "            </accessor>\n";
        o << "          </technique_common>\n";
        o << "        </source>\n";
    }

    void WriteMeshNormalsSource(std::ostream& o, DAEGeometry const& geom)
    {
        auto const vals = geom.mesh.getNormals();
        size_t const floatCount = 3 * vals.size();
        size_t const normalCount = vals.size();

        o << "        <source id=\""  << geom.geometryID << "-normals\">\n";
        o << "          <float_array id=\"" << geom.geometryID << "-normals-array\" count=\"" << floatCount << "\">" << ToFloatSpan(vals) << "</float_array>\n";
        o << "          <technique_common>\n";
        o << "            <accessor source=\"#" << geom.geometryID << "-normals-array\" count=\"" << normalCount << "\" stride=\"3\">\n";
        o << "              <param name=\"X\" type=\"float\"/>\n";
        o << "              <param name=\"Y\" type=\"float\"/>\n";
        o << "              <param name=\"Z\" type=\"float\"/>\n";
        o << "            </accessor>\n";
        o << "          </technique_common>\n";
        o << "        </source>\n";
    }

    void WriteMeshTextureCoordsSource(std::ostream& o, DAEGeometry const& geom)
    {
        auto const vals = geom.mesh.getTexCoords();
        size_t const floatCount = 2 * vals.size();
        size_t const coordCount = vals.size();

        o << "        <source id=\"" << geom.geometryID << "-map-0\">\n";
        o << "          <float_array id=\"" << geom.geometryID << "-map-0-array\" count=\"" << floatCount << "\">" << ToFloatSpan(vals) <<  "</float_array>\n";
        o << "          <technique_common>\n";
        o << "            <accessor source=\"#" << geom.geometryID << "-map-0-array\" count=\"" << coordCount << "\" stride=\"2\">\n";
        o << "              <param name=\"S\" type=\"float\"/>\n";
        o << "              <param name=\"T\" type=\"float\"/>\n";
        o << "            </accessor>\n";
        o << "          </technique_common>\n";
        o << "        </source>\n";
    }

    void WriteMeshVertices(std::ostream& o, DAEGeometry const& geom)
    {
        o << "        <vertices id=\"" << geom.geometryID << "-vertices\">\n";
        o << R"(           <input semantic="POSITION" source="#)" << geom.geometryID << "-positions\"/>\n";
        o << "        </vertices>\n";
    }

    void WriteMeshTriangles(std::ostream& o, DAEGeometry const& geom)
    {
        auto const indices = geom.mesh.getIndices();
        size_t const numTriangles = indices.size() / 3;

        o << "        <triangles count=\"" << numTriangles << "\">\n";
        o << R"(            <input semantic="VERTEX" source="#)" << geom.geometryID << "-vertices\" offset=\"0\" />\n";
        if (geom.mesh.hasNormals())
        {
            o << R"(            <input semantic="NORMAL" source="#)" << geom.geometryID << "-normals\" offset=\"0\" />\n";
        }
        if (geom.mesh.hasTexCoords())
        {
            o << R"(            <input semantic="TEXCOORD" source="#)" << geom.geometryID << "-map-0\" offset=\"0\" set=\"0\"/>\n";
        }

        o << "          <p>";
        std::string_view delim;
        for (uint32_t v : indices)
        {
            o << delim << v;
            delim = " ";
        }
        o << "</p>\n";
        o << "        </triangles>\n";
    }

    void WriteMesh(std::ostream& o, DAEGeometry const& geom)
    {
        o << R"(      <mesh>)";
        o << '\n';

        WriteMeshPositionsSource(o, geom);
        if (geom.mesh.hasNormals())
        {
            WriteMeshNormalsSource(o, geom);
        }
        if (geom.mesh.hasTexCoords())
        {
            WriteMeshTextureCoordsSource(o, geom);
        }
        WriteMeshVertices(o, geom);
        WriteMeshTriangles(o, geom);

        o << R"(      </mesh>)";
        o << '\n';
    }

    void WriteGeometry(std::ostream& o, DAEGeometry const& geom)
    {
        o << "    <geometry id=\"" << geom.geometryID << "\" name=\"" << geom.geometryID << "\">\n";
        WriteMesh(o, geom);
        o << "    </geometry>\n";
    }

    void WriteLibraryGeometries(std::ostream& o, std::span<DAEGeometry const> geoms)
    {
        o << "  <library_geometries>\n";
        for (DAEGeometry const& geom : geoms)
        {
            WriteGeometry(o, geom);
        }
        o << "  </library_geometries>\n";
    }

    void WriteTransformMatrix(std::ostream& o, Transform const& t)
    {
        Mat4 const m = ToMat4(t);

        // row-major
        o << R"(        <matrix sid="transform">)";
        std::string_view delim;
        for (Mat4::length_type row = 0; row < 4; ++row)
        {
            o << delim << m[0][row];
            delim = " ";
            o << delim << m[1][row];
            o << delim << m[2][row];
            o << delim << m[3][row];
        }
        o << R"(</matrix>)";
        o << '\n';
    }

    void WriteInstanceGeometryBindMaterial(std::ostream& o, DAEInstance const& instance)
    {
        o << "          <bind_material>\n";
        o << "            <technique_common>\n";
        o << "              <instance_material symbol=\"" << instance.materialID << "-material\" target=\"#" << instance.materialID << "-material\" />\n";
        o << "            </technique_common>\n";
        o << "          </bind_material>\n";
    }

    void WriteNodeInstanceGeometry(std::ostream& o, DAEInstance const& instance)
    {
        o << "        <instance_geometry url=\"#" << instance.geometryID << "\" name=\"" << instance.geometryID << "\">\n";
        WriteInstanceGeometryBindMaterial(o, instance);
        o << "        </instance_geometry>\n";
    }

    void WriteSceneNode(std::ostream& o, DAEInstance const& instance)
    {
        o << "      <node id=\"" << instance.instanceID << "\" name=\"" << instance.instanceID << "\" type=\"NODE\">\n";
        WriteTransformMatrix(o, instance.transform);
        WriteNodeInstanceGeometry(o, instance);
        o << "      </node>\n";
    }

    void WriteMainScene(std::ostream& o, DAESceneGraph const& graph)
    {
        o << R"(  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">)";
        o << '\n';

        for (DAEInstance const& ins : graph.instances)
        {
            WriteSceneNode(o, ins);
        }

        o << R"(    </visual_scene>
  </library_visual_scenes>)";
        o << '\n';
    }
}


// public API

osc::DAEMetadata::DAEMetadata(
    std::string_view author_,
    std::string_view authoringTool_) :
    author{author_},
    authoringTool{authoringTool_},
    creationTime{GetSystemCalendarTime()},
    modificationTime{creationTime}
{
}

void osc::WriteDecorationsAsDAE(
    std::ostream& o,
    std::span<SceneDecoration const> els,
    DAEMetadata const& metadata)
{
    DAESceneGraph const graph = ToDAESceneGraph(els);

    WriteXMLHeader(o);
    WriteCOLLADARootNodeBEGIN(o);
    WriteTopLevelAssetBlock(o, metadata);
    WriteLibraryEffects(o, graph.materials);
    WriteLibraryMaterials(o, graph.materials);
    WriteLibraryGeometries(o, graph.geometries);
    WriteMainScene(o, graph);
    WriteCOLLADARootNodeEND(o);
}

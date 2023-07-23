#include "DAE.hpp"

#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/SceneDecoration.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Platform/os.hpp"
#include "OscarConfiguration.hpp"

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

// scene-to-graph conversion stuff
namespace
{
    struct DAEGeometry final {

        DAEGeometry(std::string geometryID_, osc::Mesh const& mesh_) :
            geometryID{std::move(geometryID_)},
            mesh{mesh_}
        {
        }

        std::string geometryID;
        osc::Mesh mesh;
    };

    struct DAEMaterial final {

        DAEMaterial(
            std::string materialID_,
            osc::Color const& color_) :

            materialID{std::move(materialID_)},
            color{color_}
        {
        }

        std::string materialID;
        osc::Color color;
    };

    struct DAEInstance final {

        DAEInstance(
            std::string instanceID_,
            std::string geometryID_,
            std::string materialID_,
            osc::Transform const& transform_) :

            instanceID{std::move(instanceID_)},
            geometryID{std::move(geometryID_)},
            materialID{std::move(materialID_)},
            transform{transform_}
        {
        }

        std::string instanceID;
        std::string geometryID;
        std::string materialID;
        osc::Transform transform;
    };

    // internal representation of a datastructure that more closely resembles
    // how DAE files are structured
    struct DAESceneGraph final {
        std::vector<DAEGeometry> geometries;
        std::vector<DAEMaterial> materials;
        std::vector<DAEInstance> instances;
    };

    DAESceneGraph ToDAESceneGraph(nonstd::span<osc::SceneDecoration const> els)
    {
        DAESceneGraph rv;

        int64_t latestMesh = 0;
        int64_t latestMaterial = 0;
        std::unordered_map<osc::Mesh, std::string> mesh2id;
        std::unordered_map<osc::Color, std::string> color2materialid;
        int64_t latestInstance = 0;

        for (osc::SceneDecoration const& el : els)
        {
            if (el.mesh.getTopology() != osc::MeshTopology::Triangles)
            {
                continue;  // unsupported
            }

            auto [meshIt, meshInserted] = mesh2id.try_emplace(el.mesh, "");
            if (meshInserted)
            {
                std::stringstream id;
                id << "mesh_" << latestMesh++;
                meshIt->second = std::move(id).str();

                rv.geometries.emplace_back(meshIt->second, meshIt->first);
            }

            auto [materialIt, materialInserted] = color2materialid.try_emplace(el.color, "");
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
    nonstd::span<float const> ToFloatSpan(nonstd::span<glm::vec2 const> s)
    {
        return {glm::value_ptr(s[0]), 2 * s.size()};
    }

    nonstd::span<float const> ToFloatSpan(nonstd::span<glm::vec3 const> s)
    {
        return {glm::value_ptr(s[0]), 3 * s.size()};
    }

    nonstd::span<float const> ToFloatSpan(nonstd::span<osc::Color const> s)
    {
        return {osc::ValuePtr(s[0]), 4 * s.size()};
    }

    nonstd::span<float const> ToFloatSpan(glm::vec2 const& v)
    {
        return {glm::value_ptr(v), 2};
    }

    nonstd::span<float const> ToFloatSpan(glm::vec3 const& v)
    {
        return {glm::value_ptr(v), 3};
    }

    nonstd::span<float const> ToFloatSpan(osc::Color const& v)
    {
        return {osc::ValuePtr(v), 4};
    }

    template<typename T>
    std::string ToDaeList(nonstd::span<T const> vs)
    {
        std::stringstream ss;
        std::string_view delim;
        for (float v : vs)
        {
            ss << delim << v;
            delim = " ";
        }
        return std::move(ss).str();
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

    void WriteTopLevelAssetBlock(std::ostream& o, osc::DAEMetadata const& metadata)
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
        o << "              <color sid=\"diffuse\">" << ToDaeList(ToFloatSpan(material.color)) << "</color>\n";
        o << "            </diffuse>\n";
        o << "            <reflectivity>\n";
        o << "              <float sid=\"specular\">0.0</float>\n";
        o << "            </reflectivity>\n";
        o << "          </lambert>\n";
        o << "        </technique>\n";
        o << "      </profile_COMMON>\n";
        o << "    </effect>\n";
    }

    void WriteLibraryEffects(std::ostream& o, nonstd::span<DAEMaterial const> materials)
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

    void WriteLibraryMaterials(std::ostream& o, nonstd::span<DAEMaterial const> materials)
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
        nonstd::span<glm::vec3 const> const vals = geom.mesh.getVerts();
        size_t const floatCount = 3 * vals.size();
        size_t const vertCount = vals.size();

        o << "        <source id=\"" << geom.geometryID << "-positions\">\n";
        o << "          <float_array id=\"" << geom.geometryID << "-positions-array\" count=\"" << floatCount << "\">" << ToDaeList(ToFloatSpan(vals)) << "</float_array>\n";
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
        nonstd::span<glm::vec3 const> const vals = geom.mesh.getNormals();
        size_t const floatCount = 3 * vals.size();
        size_t const normalCount = vals.size();

        o << "        <source id=\""  << geom.geometryID << "-normals\">\n";
        o << "          <float_array id=\"" << geom.geometryID << "-normals-array\" count=\"" << floatCount << "\">" << ToDaeList(ToFloatSpan(vals)) << "</float_array>\n";
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
        nonstd::span<glm::vec2 const> const vals = geom.mesh.getTexCoords();
        size_t const floatCount = 2 * vals.size();
        size_t const coordCount = vals.size();

        o << "        <source id=\"" << geom.geometryID << "-map-0\">\n";
        o << "          <float_array id=\"" << geom.geometryID << "-map-0-array\" count=\"" << floatCount << "\">" << ToDaeList(ToFloatSpan(vals)) <<  "</float_array>\n";
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
        osc::MeshIndicesView const indices = geom.mesh.getIndices();
        size_t const numTriangles = indices.size() / 3;

        o << "        <triangles count=\"" << numTriangles << "\">\n";
        o << R"(            <input semantic="VERTEX" source="#)" << geom.geometryID << "-vertices\" offset=\"0\" />\n";
        if (!geom.mesh.getNormals().empty())
        {
            o << R"(            <input semantic="NORMAL" source="#)" << geom.geometryID << "-normals\" offset=\"0\" />\n";
        }
        if (!geom.mesh.getTexCoords().empty())
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
        if (!geom.mesh.getNormals().empty())
        {
            WriteMeshNormalsSource(o, geom);
        }
        if (!geom.mesh.getTexCoords().empty())
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

    void WriteLibraryGeometries(std::ostream& o, nonstd::span<DAEGeometry const> geoms)
    {
        o << "  <library_geometries>\n";
        for (DAEGeometry const& geom : geoms)
        {
            WriteGeometry(o, geom);
        }
        o << "  </library_geometries>\n";
    }

    void WriteTransformMatrix(std::ostream& o, osc::Transform const& t)
    {
        glm::mat4 const m = osc::ToMat4(t);

        // row-major
        o << R"(        <matrix sid="transform">)";
        std::string_view delim;
        for (glm::mat4::length_type row = 0; row < 4; ++row)
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

    void WriteSceneList(std::ostream& o)
    {
        o << R"(
  <scene>
    <instance_visual_scene url="#Scene"/>
  </scene>)";
        o << '\n';
    }
}


// public API

osc::DAEMetadata::DAEMetadata() :
    author{OSC_APPNAME_STRING},
    authoringTool{OSC_APPNAME_STRING " v" OSC_VERSION_STRING " (build " OSC_BUILD_ID ")"},
    creationTime{GetSystemCalendarTime()},
    modificationTime{creationTime}
{
}

void osc::WriteDecorationsAsDAE(
    std::ostream& o,
    nonstd::span<SceneDecoration const> els,
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

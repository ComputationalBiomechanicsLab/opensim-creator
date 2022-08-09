#include "Mesh.hpp"

#include "src/Graphics/Gl.hpp"
#include "src/Graphics/MeshData.hpp"
#include "src/Graphics/MeshTopography.hpp"
#include "src/Graphics/ShaderLocationIndex.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/BVH.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Line.hpp"
#include "src/Maths/RayCollision.hpp"

#include <glm/mat4x4.hpp>
#include <glm/mat4x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <nonstd/span.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

union PackedIndex {
    uint32_t u32;
    struct U16Pack { uint16_t a; uint16_t b; } u16;
};

static_assert(sizeof(PackedIndex) == sizeof(uint32_t));
static_assert(alignof(PackedIndex) == alignof(uint32_t));

static std::string GenerateName()
{
    static std::atomic<int> g_NextSuffix = 0;
    std::stringstream ss;
    ss << "Mesh_" << g_NextSuffix++;
    return std::move(ss).str();
}

class osc::Mesh::Impl final {
public:
    std::string name = GenerateName();
    MeshTopography topography = MeshTopography::Triangles;
    std::vector<glm::vec3> verts{};
    std::vector<glm::vec3> normals{};
    std::vector<glm::vec2> texCoords{};
    IndexFormat indexFormat = IndexFormat::UInt16;
    int numIndices = 0;
    std::unique_ptr<PackedIndex[]> indicesData = nullptr;
    AABB aabb{};
    BVH triangleBVH{};
    bool gpuBuffersOutOfDate = false;

    // lazily-loaded on request, so that non-UI threads can make Meshes
    std::optional<gl::TypedBufferHandle<GL_ARRAY_BUFFER>> maybeVBO;
    std::optional<gl::TypedBufferHandle<GL_ELEMENT_ARRAY_BUFFER>> maybeEBO;
    std::optional<gl::VertexArray> maybeVAO;
};

static bool isGreaterThanU16Max(uint32_t v)
{
    return v > std::numeric_limits<uint16_t>::max();
}

static bool anyIndicesGreaterThanU16Max(nonstd::span<uint32_t const> vs)
{
    return std::any_of(vs.begin(), vs.end(), isGreaterThanU16Max);
}

static std::unique_ptr<PackedIndex[]> repackU32IndicesToU16(nonstd::span<uint32_t const> vs)
{
    int srcN = static_cast<int>(vs.size());
    int destN = (srcN+1)/2;
    std::unique_ptr<PackedIndex[]> rv{new PackedIndex[destN]};

    if (srcN == 0)
    {
        return rv;
    }

    for (int srcIdx = 0, end = srcN-2; srcIdx < end; srcIdx += 2)
    {
        int destIdx = srcIdx/2;
        rv[destIdx].u16.a = static_cast<uint16_t>(vs[srcIdx]);
        rv[destIdx].u16.b = static_cast<uint16_t>(vs[srcIdx+1]);
    }

    if (srcN % 2 != 0)
    {
        int destIdx = destN-1;
        rv[destIdx].u16.a = static_cast<uint16_t>(vs[srcN-1]);
        rv[destIdx].u16.b = 0x0000;
    }
    else
    {
        int destIdx = destN-1;
        rv[destIdx].u16.a = static_cast<uint16_t>(vs[srcN-2]);
        rv[destIdx].u16.b = static_cast<uint16_t>(vs[srcN-1]);
    }

    return rv;
}

static std::unique_ptr<PackedIndex[]> unpackU16IndicesToU32(nonstd::span<uint16_t const> vs)
{
    std::unique_ptr<PackedIndex[]> rv{new PackedIndex[vs.size()]};

    for (size_t i = 0; i < vs.size(); ++i)
    {
        rv[i].u32 = static_cast<uint32_t>(vs[i]);
    }

    return rv;
}

static std::unique_ptr<PackedIndex[]> copyU32IndicesToU32(nonstd::span<uint32_t const> vs)
{
    std::unique_ptr<PackedIndex[]> rv{new PackedIndex[vs.size()]};

    PackedIndex const* start = reinterpret_cast<PackedIndex const*>(vs.data());
    PackedIndex const* end = start + vs.size();
    std::copy(start, end, rv.get());
    return rv;
}

static std::unique_ptr<PackedIndex[]> copyU16IndicesToU16(nonstd::span<uint16_t const> vs)
{
    int srcN = static_cast<int>(vs.size());
    int destN = (srcN+1)/2;
    std::unique_ptr<PackedIndex[]> rv{new PackedIndex[destN]};

    if (srcN == 0)
    {
        return rv;
    }

    uint16_t* ptr = &rv[0].u16.a;
    std::copy(vs.data(), vs.data() + vs.size(), ptr);

    if (srcN % 2)
    {
        ptr[destN-1] = 0x0000;
    }

    return rv;
}

static nonstd::span<uint32_t const> AsU32Span(PackedIndex const* pi, size_t n)
{
    return nonstd::span{&pi->u32, n};
}

static nonstd::span<uint16_t const> AsU16Span(PackedIndex const* pi, size_t n)
{
    return nonstd::span{&pi->u16.a, n};
}

osc::Mesh::Mesh(MeshData cpuMesh) :
    m_Impl{new Impl{}}
{
    m_Impl->topography = cpuMesh.topography;
    m_Impl->verts = std::move(cpuMesh.verts);
    m_Impl->normals = std::move(cpuMesh.normals);
    m_Impl->texCoords = std::move(cpuMesh.texcoords);

    // repack indices (if necessary)
    m_Impl->indexFormat = anyIndicesGreaterThanU16Max(cpuMesh.indices) ? IndexFormat::UInt32 : IndexFormat::UInt16;
    m_Impl->numIndices = static_cast<int>(cpuMesh.indices.size());
    if (m_Impl->indexFormat == IndexFormat::UInt32)
    {
        m_Impl->indicesData = copyU32IndicesToU32(cpuMesh.indices);
    }
    else
    {
        m_Impl->indicesData = repackU32IndicesToU16(cpuMesh.indices);
    }

    this->recalculateBounds();

    m_Impl->gpuBuffersOutOfDate = true;
}

osc::Mesh::Mesh(Mesh&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::Mesh& osc::Mesh::operator=(Mesh&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::Mesh::~Mesh() noexcept
{
    delete m_Impl;
}

osc::MeshTopography osc::Mesh::getTopography() const
{
    return m_Impl->topography;
}

GLenum osc::Mesh::getTopographyOpenGL() const
{
    switch (m_Impl->topography) {
    case MeshTopography::Triangles:
        return GL_TRIANGLES;
    case MeshTopography::Lines:
        return GL_LINES;
    default:
        throw std::runtime_error{"unsuppored topography"};
    }
}

void osc::Mesh::setTopography(MeshTopography t)
{
    m_Impl->topography = t;
}

nonstd::span<glm::vec3 const> osc::Mesh::getVerts() const
{
    return m_Impl->verts;
}

void osc::Mesh::setVerts(nonstd::span<const glm::vec3> vs)
{
    auto& verts = m_Impl->verts;
    verts.clear();
    verts.reserve(vs.size());
    for (glm::vec3 const& v : vs)
    {
        verts.push_back(v);
    }

    recalculateBounds();
    m_Impl->gpuBuffersOutOfDate = true;
}

nonstd::span<glm::vec3 const> osc::Mesh::getNormals() const
{
    return m_Impl->normals;
}

void osc::Mesh::setNormals(nonstd::span<const glm::vec3> ns)
{
    auto& norms = m_Impl->normals;
    norms.clear();
    norms.reserve(ns.size());
    for (glm::vec3 const& v : ns) {
        norms.push_back(v);
    }

    m_Impl->gpuBuffersOutOfDate = true;
}

nonstd::span<glm::vec2 const> osc::Mesh::getTexCoords() const
{
    return m_Impl->texCoords;
}

void osc::Mesh::setTexCoords(nonstd::span<const glm::vec2> tc)
{
    auto& coords = m_Impl->texCoords;
    coords.clear();
    coords.reserve(tc.size());
    for (glm::vec2 const& t : tc)
    {
        coords.push_back(t);
    }

    m_Impl->gpuBuffersOutOfDate = true;
}

void osc::Mesh::scaleTexCoords(float factor)
{
    for (auto& tc : m_Impl->texCoords)
    {
        tc *= factor;
    }
    m_Impl->gpuBuffersOutOfDate = true;
}

osc::IndexFormat osc::Mesh::getIndexFormat() const
{
    return m_Impl->indexFormat;
}

GLenum osc::Mesh::getIndexFormatOpenGL() const
{
    return m_Impl->indexFormat == IndexFormat::UInt16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}

void osc::Mesh::setIndexFormat(IndexFormat newFormat)
{
    IndexFormat oldFormat = m_Impl->indexFormat;

    if (newFormat == oldFormat)
    {
        return;
    }

    m_Impl->indexFormat = newFormat;

    // format changed: need to pack/unpack the data
    PackedIndex const* pis = m_Impl->indicesData.get();

    if (newFormat == IndexFormat::UInt16)
    {
        uint32_t const* existing = &pis[0].u32;
        nonstd::span<uint32_t const> s(existing, m_Impl->numIndices);
        m_Impl->indicesData = repackU32IndicesToU16(s);
    }
    else
    {
        uint16_t const* existing = &pis[0].u16.a;
        nonstd::span<uint16_t const> s(existing, m_Impl->numIndices);
        m_Impl->indicesData = unpackU16IndicesToU32(s);
    }

    m_Impl->gpuBuffersOutOfDate = true;
}

int osc::Mesh::getNumIndices() const
{
    return m_Impl->numIndices;
}

std::vector<uint32_t> osc::Mesh::getIndices() const
{
    int numIndices = m_Impl->numIndices;
    PackedIndex const* pis = m_Impl->indicesData.get();

    if (m_Impl->indexFormat == IndexFormat::UInt16)
    {
        uint16_t const* ptr = &pis[0].u16.a;
        nonstd::span<uint16_t const> s(ptr, numIndices);

        std::vector<uint32_t> rv;
        rv.reserve(numIndices);
        for (uint16_t v : s)
        {
            rv.push_back(static_cast<uint32_t>(v));
        }

        return rv;
    }
    else
    {
        uint32_t const* ptr = &pis[0].u32;
        nonstd::span<uint32_t const> s(ptr, numIndices);

        return std::vector<uint32_t>(s.begin(), s.end());
    }
}

void osc::Mesh::setIndicesU16(nonstd::span<const uint16_t> vs)
{
    if (m_Impl->indexFormat == IndexFormat::UInt16)
    {
        m_Impl->indicesData = copyU16IndicesToU16(vs);
    }
    else
    {
        m_Impl->indicesData = unpackU16IndicesToU32(vs);
    }

    recalculateBounds();
    m_Impl->numIndices = static_cast<int>(vs.size());
    m_Impl->gpuBuffersOutOfDate = true;
}

void osc::Mesh::setIndicesU32(nonstd::span<const uint32_t> vs)
{
    if (m_Impl->indexFormat == IndexFormat::UInt16)
    {
        m_Impl->indicesData = repackU32IndicesToU16(vs);
    }
    else
    {
        m_Impl->indicesData = copyU32IndicesToU32(vs);
    }

    recalculateBounds();
    m_Impl->numIndices = static_cast<int>(vs.size());
    m_Impl->gpuBuffersOutOfDate = true;
}

osc::AABB const& osc::Mesh::getAABB() const
{
    return m_Impl->aabb;
}

osc::AABB osc::Mesh::getWorldspaceAABB(Transform const& localToWorldXform) const
{
    return TransformAABB(m_Impl->aabb, ToMat4(localToWorldXform));
}

osc::AABB osc::Mesh::getWorldspaceAABB(glm::mat4x3 const& modelMatrix) const
{
    return TransformAABB(m_Impl->aabb, modelMatrix);
}

osc::BVH const& osc::Mesh::getTriangleBVH() const
{
    return m_Impl->triangleBVH;
}

osc::RayCollision osc::Mesh::getClosestRayTriangleCollisionModelspace(Line const& ray) const
{
    if (m_Impl->topography != MeshTopography::Triangles)
    {
        return RayCollision{false, 0.0f};
    }

    BVHCollision coll;
    bool collided;

    if (m_Impl->indexFormat == IndexFormat::UInt16)
    {
        auto indices = AsU16Span(m_Impl->indicesData.get(), m_Impl->numIndices);
        collided = BVH_GetClosestRayIndexedTriangleCollision(
            m_Impl->triangleBVH,
            m_Impl->verts,
            indices,
            ray,
            &coll);
    }
    else
    {
        auto indices = AsU32Span(m_Impl->indicesData.get(), m_Impl->numIndices);
        collided = BVH_GetClosestRayIndexedTriangleCollision(
            m_Impl->triangleBVH,
            m_Impl->verts,
            indices,
            ray,
            &coll);
    }

    if (collided)
    {
        return RayCollision{true, coll.distance};
    }
    else
    {
        return RayCollision{false, 0.0f};
    }
}

osc::RayCollision osc::Mesh::getRayMeshCollisionInWorldspace(glm::mat4 const& model2world, Line const& worldspaceLine) const
{
    // do a fast ray-to-AABB collision test
    AABB modelspaceAABB = getAABB();
    AABB worldspaceAABB = TransformAABB(modelspaceAABB, model2world);

    RayCollision rayAABBCollision = GetRayCollisionAABB(worldspaceLine, worldspaceAABB);

    if (!rayAABBCollision.hit)
    {
        return rayAABBCollision;  // missed the AABB, so *definitely* missed the mesh
    }

    // it hit the AABB, so it *may* have hit a triangle in the mesh
    //
    // refine the hittest by doing a slower ray-to-triangle test
    glm::mat4 world2model = glm::inverse(model2world);
    Line modelspaceLine = TransformLine(worldspaceLine, world2model);

    return getClosestRayTriangleCollisionModelspace(modelspaceLine);
}

osc::RayCollision osc::Mesh::getRayMeshCollisionInWorldspace(Transform const& model2world, Line const& worldspaceLine) const
{
    // do a fast ray-to-AABB collision test
    AABB modelspaceAABB = getAABB();
    AABB worldspaceAABB = TransformAABB(modelspaceAABB, model2world);

    RayCollision rayAABBCollision = GetRayCollisionAABB(worldspaceLine, worldspaceAABB);

    if (!rayAABBCollision.hit)
    {
        return rayAABBCollision;  // missed the AABB, so *definitely* missed the mesh
    }

    // it hit the AABB, so it *may* have hit a triangle in the mesh
    //
    // refine the hittest by doing a slower ray-to-triangle test
    glm::mat4 world2model = osc::ToInverseMat4(model2world);
    Line modelspaceLine = TransformLine(worldspaceLine, world2model);

    return getClosestRayTriangleCollisionModelspace(modelspaceLine);
}

void osc::Mesh::clear()
{
    m_Impl->verts.clear();
    m_Impl->normals.clear();
    m_Impl->texCoords.clear();
    m_Impl->numIndices = 0;
    m_Impl->indicesData.reset();
    m_Impl->aabb = {};
    m_Impl->triangleBVH.clear();
    m_Impl->gpuBuffersOutOfDate = true;
    m_Impl->maybeVBO = std::nullopt;
    m_Impl->maybeVAO = std::nullopt;
}

void osc::Mesh::recalculateBounds()
{
    if (m_Impl->indexFormat == IndexFormat::UInt16)
    {
        auto indices = AsU16Span(m_Impl->indicesData.get(), m_Impl->numIndices);
        m_Impl->aabb = AABBFromIndexedVerts(m_Impl->verts, indices);
        if (m_Impl->topography == MeshTopography::Triangles)
        {
            BVH_BuildFromIndexedTriangles(m_Impl->triangleBVH, m_Impl->verts, indices);
        }
        else
        {
            m_Impl->triangleBVH.clear();
        }
    }
    else
    {
        auto indices = AsU32Span(m_Impl->indicesData.get(), m_Impl->numIndices);
        m_Impl->aabb = AABBFromIndexedVerts(m_Impl->verts, indices);
        if (m_Impl->topography == MeshTopography::Triangles)
        {
            BVH_BuildFromIndexedTriangles(m_Impl->triangleBVH, m_Impl->verts, indices);
        }
        else
        {
            m_Impl->triangleBVH.clear();
        }
    }
}

void osc::Mesh::uploadToGPU()
{
    // pack CPU-side mesh data (verts, etc.), which is separate, into a
    // suitable GPU-side buffer
    nonstd::span<glm::vec3 const> verts = getVerts();
    nonstd::span<glm::vec3 const> normals = getNormals();
    nonstd::span<glm::vec2 const> uvs = getTexCoords();

    bool hasNormals = !normals.empty();
    bool hasUvs = !uvs.empty();

    if (hasNormals && normals.size() != verts.size())
    {
        throw std::runtime_error{"number of normals != number of verts"};
    }

    if (hasUvs && uvs.size() != verts.size())
    {
        throw std::runtime_error{"number of uvs != number of verts"};
    }

    size_t stride = sizeof(decltype(verts)::element_type);
    if (hasNormals)
    {
        stride += sizeof(decltype(normals)::element_type);
    }
    if (hasUvs)
    {
        stride += sizeof(decltype(uvs)::element_type);
    }

    std::vector<unsigned char> data;
    data.reserve(stride * verts.size());

    auto pushFloat = [&data](float v)
    {
        data.push_back(reinterpret_cast<unsigned char*>(&v)[0]);
        data.push_back(reinterpret_cast<unsigned char*>(&v)[1]);
        data.push_back(reinterpret_cast<unsigned char*>(&v)[2]);
        data.push_back(reinterpret_cast<unsigned char*>(&v)[3]);
    };

    for (size_t i = 0; i < verts.size(); ++i)
    {
        pushFloat(verts[i].x);
        pushFloat(verts[i].y);
        pushFloat(verts[i].z);
        if (hasNormals)
        {
            pushFloat(normals[i].x);
            pushFloat(normals[i].y);
            pushFloat(normals[i].z);
        }
        if (hasUvs)
        {
            pushFloat(uvs[i].x);
            pushFloat(uvs[i].y);
        }
    }

    if (data.size() != stride * verts.size())
    {
        throw std::runtime_error{"unexpected size"};
    }

    // allocate VBO handle on GPU if not-yet allocated. Upload the
    // data to the VBO
    if (!m_Impl->maybeVBO)
    {
        m_Impl->maybeVBO = gl::TypedBufferHandle<GL_ARRAY_BUFFER>{};
    }
    gl::TypedBufferHandle<GL_ARRAY_BUFFER>& vbo = *m_Impl->maybeVBO;
    gl::BindBuffer(GL_ARRAY_BUFFER, vbo);
    gl::BufferData(GL_ARRAY_BUFFER, data.size(), data.data(), GL_STATIC_DRAW);

    // allocate EBO handle on GPU if not-yet allocated. Upload the
    // data to the EBO
    if (!m_Impl->maybeEBO)
    {
        m_Impl->maybeEBO = gl::TypedBufferHandle<GL_ELEMENT_ARRAY_BUFFER>{};
    }
    gl::TypedBufferHandle<GL_ELEMENT_ARRAY_BUFFER>& ebo = *m_Impl->maybeEBO;
    gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    size_t eboSize = m_Impl->indexFormat == IndexFormat::UInt16 ? sizeof(uint16_t) * m_Impl->numIndices : sizeof(uint32_t) * m_Impl->numIndices;
    gl::BufferData(GL_ELEMENT_ARRAY_BUFFER, eboSize, m_Impl->indicesData.get(), GL_STATIC_DRAW);

    // always allocate a new VAO in case the old one has stuff lying around
    // in it from an old format
    //
    // upload the packing format to the VAO
    m_Impl->maybeVAO = gl::VertexArray{};
    gl::VertexArray& vao = *m_Impl->maybeVAO;

    gl::BindVertexArray(vao);
    gl::BindBuffer(GL_ARRAY_BUFFER, vbo);
    gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    int offset = 0;
    auto int2void = [](int v)
    {
        return reinterpret_cast<void*>(static_cast<uintptr_t>(v));
    };

    glVertexAttribPointer(SHADER_LOC_VERTEX_POSITION, 3, GL_FLOAT, false, static_cast<GLsizei>(stride), int2void(offset));
    glEnableVertexAttribArray(SHADER_LOC_VERTEX_POSITION);
    offset += 3 * sizeof(float);

    if (hasNormals)
    {
        glVertexAttribPointer(SHADER_LOC_VERTEX_NORMAL, 3, GL_FLOAT, false, static_cast<GLsizei>(stride), int2void(offset));
        glEnableVertexAttribArray(SHADER_LOC_VERTEX_NORMAL);
        offset += 3 * sizeof(float);
    }

    if (hasUvs)
    {
        glVertexAttribPointer(SHADER_LOC_VERTEX_TEXCOORD01, 2, GL_FLOAT, false, static_cast<GLsizei>(stride), int2void(offset));
        glEnableVertexAttribArray(SHADER_LOC_VERTEX_TEXCOORD01);
    }

    gl::BindVertexArray();

    m_Impl->gpuBuffersOutOfDate = false;
}

gl::VertexArray& osc::Mesh::GetVertexArray()
{
    if (m_Impl->gpuBuffersOutOfDate
        || !m_Impl->maybeVBO
        || !m_Impl->maybeVAO
        || !m_Impl->maybeEBO)
    {
        uploadToGPU();
    }

    return *m_Impl->maybeVAO;
}

void osc::Mesh::Draw()
{
    gl::DrawElements(getTopographyOpenGL(), getNumIndices(), getIndexFormatOpenGL(), nullptr);
}

void osc::Mesh::DrawInstanced(size_t n)
{
    glDrawElementsInstanced(getTopographyOpenGL(),
                            getNumIndices(),
                            getIndexFormatOpenGL(),
                            nullptr,
                            static_cast<GLsizei>(n));
}

#include "Mesh.hpp"

#include "src/3D/Gl.hpp"
#include "src/3D/ShaderLocationIndex.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <nonstd/span.hpp>

#include <atomic>
#include <memory>
#include <optional>
#include <vector>

using namespace osc;

static std::atomic<Mesh::IdType> g_LatestId = 1;

union PackedIndex {
    uint32_t u32;
    struct U16Pack { uint16_t a; uint16_t b; } u16;
};

static_assert(sizeof(PackedIndex) == sizeof(uint32_t));
static_assert(alignof(PackedIndex) == alignof(uint32_t));

struct osc::Mesh::Impl final {
    Mesh::IdType id;
    MeshTopography topography;
    std::vector<glm::vec3> verts;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    IndexFormat indexFormat;
    int numIndices;
    std::unique_ptr<PackedIndex[]> indicesData;
    AABB aabb;
    Sphere boundingSphere;
    BVH triangleBVH;
    bool gpuBuffersOutOfDate;

    // lazily-loaded on request, so that non-UI threads can make Meshes
    std::optional<gl::TypedBufferHandle<GL_ARRAY_BUFFER>> maybeVBO;
    std::optional<gl::TypedBufferHandle<GL_ELEMENT_ARRAY_BUFFER>> maybeEBO;
    std::optional<gl::VertexArray> maybeVAO;

    Impl() :
        id{++g_LatestId},
        topography{MeshTopography::Triangles},
        verts{},
        normals{},
        texCoords{},
        indexFormat{IndexFormat::UInt16},
        numIndices{0},
        indicesData{nullptr},
        aabb{},
        boundingSphere{},
        triangleBVH{},
        gpuBuffersOutOfDate{false} {
    }
};

static bool isGreaterThanU16Max(uint32_t v) noexcept {
    return v > std::numeric_limits<uint16_t>::max();
}

static bool anyIndicesGreaterThanU16Max(nonstd::span<uint32_t const> vs) {
    return std::any_of(vs.begin(), vs.end(), isGreaterThanU16Max);
}

static std::unique_ptr<PackedIndex[]> repackU32IndicesToU16(nonstd::span<uint32_t const> vs) {
    int srcN = static_cast<int>(vs.size());
    int destN = (srcN+1)/2;
    std::unique_ptr<PackedIndex[]> rv{new PackedIndex[destN]};

    if (srcN == 0) {
        return rv;
    }

    for (int srcIdx = 0, end = srcN-2; srcIdx < end; srcIdx += 2) {
        int destIdx = srcIdx/2;
        rv[destIdx].u16.a = static_cast<uint16_t>(vs[srcIdx]);
        rv[destIdx].u16.b = static_cast<uint16_t>(vs[srcIdx+1]);
    }

    if (srcN % 2 != 0) {
        int destIdx = destN-1;
        rv[destIdx].u16.a = static_cast<uint16_t>(vs[srcN-1]);
        rv[destIdx].u16.b = 0x0000;
    } else {
        int destIdx = destN-1;
        rv[destIdx].u16.a = static_cast<uint16_t>(vs[srcN-2]);
        rv[destIdx].u16.b = static_cast<uint16_t>(vs[srcN-1]);
    }

    return rv;
}

static std::unique_ptr<PackedIndex[]> unpackU16IndicesToU32(nonstd::span<uint16_t const> vs)  {
    std::unique_ptr<PackedIndex[]> rv{new PackedIndex[vs.size()]};

    for (int i = 0; i < vs.size(); ++i) {
        rv[i].u32 = static_cast<uint32_t>(vs[i]);
    }

    return rv;
}

static std::unique_ptr<PackedIndex[]> copyU32IndicesToU32(nonstd::span<uint32_t const> vs) {
    std::unique_ptr<PackedIndex[]> rv{new PackedIndex[vs.size()]};

    PackedIndex const* start = reinterpret_cast<PackedIndex const*>(vs.data());
    PackedIndex const* end = start + vs.size();
    std::copy(start, end, rv.get());
    return rv;
}

static std::unique_ptr<PackedIndex[]> copyU16IndicesToU16(nonstd::span<uint16_t const> vs) {
    int srcN = static_cast<int>(vs.size());
    int destN = (srcN+1)/2;
    std::unique_ptr<PackedIndex[]> rv{new PackedIndex[destN]};

    if (srcN == 0) {
        return rv;
    }

    uint16_t* ptr = &rv[0].u16.a;
    std::copy(vs.data(), vs.data() + vs.size(), ptr);

    if (srcN % 2) {
        ptr[destN-1] = 0x0000;
    }

    return rv;
}

osc::Mesh::Mesh(MeshData cpuMesh) : m_Impl{new Impl{}} {
    m_Impl->topography = cpuMesh.topography;
    m_Impl->verts = std::move(cpuMesh.verts);
    m_Impl->normals = std::move(cpuMesh.normals);
    m_Impl->texCoords = std::move(cpuMesh.texcoords);

    // repack indices (if necessary)
    m_Impl->indexFormat = anyIndicesGreaterThanU16Max(cpuMesh.indices) ? IndexFormat::UInt32 : IndexFormat::UInt16;
    m_Impl->numIndices = cpuMesh.indices.size();
    if (m_Impl->indexFormat == IndexFormat::UInt32) {
        m_Impl->indicesData = copyU32IndicesToU32(cpuMesh.indices);
    } else {
        m_Impl->indicesData = repackU32IndicesToU16(cpuMesh.indices);
    }

    m_Impl->aabb = AABBFromVerts(m_Impl->verts.data(), m_Impl->verts.size());
    m_Impl->boundingSphere = BoundingSphereFromVerts(m_Impl->verts.data(), m_Impl->verts.size());
    BVH_BuildFromTriangles(m_Impl->triangleBVH, m_Impl->verts.data(), m_Impl->verts.size());
    m_Impl->gpuBuffersOutOfDate = true;
}

osc::Mesh::~Mesh() noexcept {
}

osc::Mesh::IdType osc::Mesh::getID() const {
    return m_Impl->id;
}

osc::MeshTopography osc::Mesh::getTopography() const {
    return m_Impl->topography;
}

GLenum osc::Mesh::getTopographyOpenGL() const {
    switch (m_Impl->topography) {
    case MeshTopography::Triangles:
        return GL_TRIANGLES;
    case MeshTopography::Lines:
        return GL_LINES;
    default:
        throw std::runtime_error{"unsuppored topography"};
    }
}

void osc::Mesh::setTopography(MeshTopography t) {
    m_Impl->topography = t;
}

nonstd::span<glm::vec3 const> osc::Mesh::getVerts() const {
    return m_Impl->verts;
}

void osc::Mesh::setVerts(nonstd::span<const glm::vec3> vs) {
    auto& verts = m_Impl->verts;
    verts.clear();
    verts.reserve(vs.size());
    for (glm::vec3 const& v : vs) {
        verts.push_back(v);
    }

    recalculateBounds();
    m_Impl->gpuBuffersOutOfDate = true;
}

nonstd::span<glm::vec3 const> osc::Mesh::getNormals() const {
    return m_Impl->normals;
}

void osc::Mesh::setNormals(nonstd::span<const glm::vec3> ns) {
    auto& norms = m_Impl->normals;
    norms.clear();
    norms.reserve(ns.size());
    for (glm::vec3 const& v : ns) {
        norms.push_back(v);
    }

    m_Impl->gpuBuffersOutOfDate = true;
}

nonstd::span<glm::vec2 const> osc::Mesh::getTexCoords() const {
    return m_Impl->texCoords;
}

void osc::Mesh::setTexCoords(nonstd::span<const glm::vec2> tc) {
    auto& coords = m_Impl->texCoords;
    coords.clear();
    coords.reserve(tc.size());
    for (glm::vec2 const& t : tc) {
        coords.push_back(t);
    }

    m_Impl->gpuBuffersOutOfDate = true;
}

IndexFormat osc::Mesh::getIndexFormat() const {
    return m_Impl->indexFormat;
}

GLenum osc::Mesh::getIndexFormatOpenGL() const {
    return m_Impl->indexFormat == IndexFormat::UInt16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}

void osc::Mesh::setIndexFormat(IndexFormat newFormat) {
    IndexFormat oldFormat = m_Impl->indexFormat;

    if (newFormat == oldFormat) {
        return;
    }

    m_Impl->indexFormat = newFormat;

    // format changed: need to pack/unpack the data
    PackedIndex const* pis = m_Impl->indicesData.get();

    if (newFormat == IndexFormat::UInt16) {
        uint32_t const* existing = &pis[0].u32;
        nonstd::span<uint32_t const> s(existing, m_Impl->numIndices);
        m_Impl->indicesData = repackU32IndicesToU16(s);
    } else {
        uint16_t const* existing = &pis[0].u16.a;
        nonstd::span<uint16_t const> s(existing, m_Impl->numIndices);
        m_Impl->indicesData = unpackU16IndicesToU32(s);
    }

    m_Impl->gpuBuffersOutOfDate = true;
}

int osc::Mesh::getNumIndices() const {
    return m_Impl->numIndices;
}

std::vector<uint32_t> osc::Mesh::getIndices() const {
    int numIndices = m_Impl->numIndices;
    PackedIndex const* pis = m_Impl->indicesData.get();

    if (m_Impl->indexFormat == IndexFormat::UInt16) {
        uint16_t const* ptr = &pis[0].u16.a;
        nonstd::span<uint16_t const> s(ptr, numIndices);

        std::vector<uint32_t> rv;
        rv.reserve(numIndices);
        for (uint16_t v : s) {
            rv.push_back(static_cast<uint32_t>(v));
        }
        return rv;
    } else {
        uint32_t const* ptr = &pis[0].u32;
        nonstd::span<uint32_t const> s(ptr, numIndices);

        return std::vector<uint32_t>(s.begin(), s.end());
    }
}

void osc::Mesh::setIndicesU16(nonstd::span<const uint16_t> vs) {
    if (m_Impl->indexFormat == IndexFormat::UInt16) {
        m_Impl->indicesData = copyU16IndicesToU16(vs);
    } else {
        m_Impl->indicesData = unpackU16IndicesToU32(vs);
    }

    recalculateBounds();
    m_Impl->numIndices = vs.size();
    m_Impl->gpuBuffersOutOfDate = true;
}

void osc::Mesh::setIndicesU32(nonstd::span<const uint32_t> vs) {
    if (m_Impl->indexFormat == IndexFormat::UInt16) {
        m_Impl->indicesData = repackU32IndicesToU16(vs);
    } else {
        m_Impl->indicesData = copyU32IndicesToU32(vs);
    }

    recalculateBounds();
    m_Impl->numIndices = vs.size();
    m_Impl->gpuBuffersOutOfDate = true;
}

AABB const& osc::Mesh::getAABB() const {
    return m_Impl->aabb;
}

Sphere const& osc::Mesh::getBoundingSphere() const {
    return m_Impl->boundingSphere;
}

std::optional<MeshCollision> osc::Mesh::getClosestRayTriangleCollision(Line const& ray) const {
    if (m_Impl->topography != MeshTopography::Triangles) {
        return std::nullopt;
    }

    BVHCollision coll;
    bool collided = BVH_GetClosestRayTriangleCollision(m_Impl->triangleBVH, m_Impl->verts.data(), m_Impl->verts.size(), ray, &coll);

    if (collided) {
        return MeshCollision{coll.distance};
    } else {
        return std::nullopt;
    }
}

void osc::Mesh::clear() {
    m_Impl->verts.clear();
    m_Impl->normals.clear();
    m_Impl->texCoords.clear();
    m_Impl->numIndices = 0;
    m_Impl->indicesData.reset();
    m_Impl->aabb = {};
    m_Impl->boundingSphere = {};
    m_Impl->triangleBVH.clear();
    m_Impl->gpuBuffersOutOfDate = true;
    m_Impl->maybeVBO = std::nullopt;
    m_Impl->maybeVAO = std::nullopt;
}

void osc::Mesh::recalculateBounds() {
    m_Impl->aabb = AABBFromVerts(m_Impl->verts.data(), m_Impl->verts.size());
    m_Impl->boundingSphere = BoundingSphereFromVerts(m_Impl->verts.data(), m_Impl->verts.size());
    BVH_BuildFromTriangles(m_Impl->triangleBVH, m_Impl->verts.data(), m_Impl->verts.size());
}

#include <iostream>
void osc::Mesh::uploadToGPU() {
    // pack CPU-side mesh data (verts, etc.), which is separate, into a
    // suitable GPU-side buffer
    nonstd::span<glm::vec3 const> verts = getVerts();
    nonstd::span<glm::vec3 const> normals = getNormals();
    nonstd::span<glm::vec2 const> uvs = getTexCoords();

    bool hasNormals = !normals.empty();
    bool hasUvs = !uvs.empty();

    if (hasNormals && normals.size() != verts.size()) {
        throw std::runtime_error{"number of normals != number of verts"};
    }

    if (hasUvs && uvs.size() != verts.size()) {
        throw std::runtime_error{"number of uvs != number of verts"};
    }

    size_t stride = sizeof(decltype(verts)::element_type);
    if (hasNormals) {
        stride += sizeof(decltype(normals)::element_type);
    }
    if (hasUvs) {
        stride += sizeof(decltype(uvs)::element_type);
    }

    std::vector<unsigned char> data;
    data.reserve(stride * verts.size());

    auto pushFloat = [&data](float v) {
        data.push_back(reinterpret_cast<unsigned char*>(&v)[0]);
        data.push_back(reinterpret_cast<unsigned char*>(&v)[1]);
        data.push_back(reinterpret_cast<unsigned char*>(&v)[2]);
        data.push_back(reinterpret_cast<unsigned char*>(&v)[3]);
    };

    for (size_t i = 0; i < verts.size(); ++i) {
        pushFloat(verts[i].x);
        pushFloat(verts[i].y);
        pushFloat(verts[i].z);
        if (hasNormals) {
            pushFloat(normals[i].x);
            pushFloat(normals[i].y);
            pushFloat(normals[i].z);
        }
        if (hasUvs) {
            pushFloat(uvs[i].x);
            pushFloat(uvs[i].y);
        }
    }

    if (data.size() != stride * verts.size()) {
        throw std::runtime_error{"unexpected size"};
    }

    // allocate VBO handle on GPU if not-yet allocated. Upload the
    // data to the VBO
    if (!m_Impl->maybeVBO) {
        m_Impl->maybeVBO = gl::TypedBufferHandle<GL_ARRAY_BUFFER>{};
    }
    gl::TypedBufferHandle<GL_ARRAY_BUFFER>& vbo = *m_Impl->maybeVBO;
    gl::BindBuffer(GL_ARRAY_BUFFER, vbo);
    gl::BufferData(GL_ARRAY_BUFFER, data.size(), data.data(), GL_STATIC_DRAW);

    // allocate EBO handle on GPU if not-yet allocated. Upload the
    // data to the EBO
    if (!m_Impl->maybeEBO) {
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

    glVertexAttribPointer(SHADER_LOC_VERTEX_POSITION, 3, GL_FLOAT, false, stride, reinterpret_cast<void*>(offset));
    glEnableVertexAttribArray(SHADER_LOC_VERTEX_POSITION);
    offset += 3 * sizeof(float);

    if (hasNormals) {
        glVertexAttribPointer(SHADER_LOC_VERTEX_NORMAL, 3, GL_FLOAT, false, stride, reinterpret_cast<void*>(offset));
        glEnableVertexAttribArray(SHADER_LOC_VERTEX_NORMAL);
        offset += 3 * sizeof(float);
    }

    if (hasUvs) {
        glVertexAttribPointer(SHADER_LOC_VERTEX_TEXCOORD01, 2, GL_FLOAT, false, stride, reinterpret_cast<void*>(offset));
        glEnableVertexAttribArray(SHADER_LOC_VERTEX_TEXCOORD01);
    }

    gl::BindVertexArray();

    m_Impl->gpuBuffersOutOfDate = false;
}

gl::VertexArray& osc::Mesh::getVAO() {
    if (m_Impl->gpuBuffersOutOfDate || !m_Impl->maybeVBO || !m_Impl->maybeVAO || !m_Impl->maybeEBO) {
        uploadToGPU();
    }

    return *m_Impl->maybeVAO;
}

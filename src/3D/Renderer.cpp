#include "Renderer.hpp"

#include "src/3D/BVH.hpp"
#include "src/Utils/UID.hpp"
#include "src/Utils/DefaultConstructOnCopy.hpp"
#include "src/Assertions.hpp"

#include <nonstd/span.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <vector>

#define OSC_THROW_NYI() throw std::runtime_error{"not yet implemented"}


// 3D implementation notes
//
// - almost all public-API types are designed to be copy-on-write classes that
//   downstream code can easily store, compare, etc. "as-if" the callers were
//   dealing with value-types
//
// - the value types are comparable, assignable, etc. so that they can be easily
//   used downstream. Most of them are also printable
//
// - hashing etc. needs to ensure a unique hash value is generated even if the
//   same implementation pointer is hashed (e.g. because the caller could be
//   using the hash as a way of caching downstream data). Over-hashing is
//   preferred to under-hashing (i.e. it's preferable for the same "value" to
//   hash to something different because something like a version number was
//   incremented, even if the values are the same, because that's better than
//   emitting the same hash value for different actual values)
//
// - all the implementation details are stuffed in here because the rendering
//   classes are going to be used all over the codebase and it's feasible that
//   little internal changes will happen here as other backends, caching, etc.
//   are supported

// globals
namespace
{
    constexpr std::array<char const*, static_cast<std::size_t>(osc::experimental::MeshTopography::TOTAL)> const g_MeshTopographyStrings =
    {
        "Triangles",
        "Lines",
    };

    constexpr std::array<char const*, static_cast<std::size_t>(osc::experimental::TextureWrapMode::TOTAL)> const g_TextureWrapModeStrings =
    {
        "Repeat",
        "Clamp",
        "Mirror",
    };

    constexpr std::array<char const*, static_cast<std::size_t>(osc::experimental::TextureFilterMode::TOTAL)> const g_TextureFilterModeStrings =
    {
        "Nearest",
        "Linear",
        "Mipmap",
    };

    constexpr std::array<char const*, static_cast<std::size_t>(osc::experimental::ShaderType::TOTAL)> g_ShaderTypeStrings =
    {
        "Float",
        "Int",
        "Matrix",
        "Texture",
        "Vector",
    };

    constexpr std::array<char const*, static_cast<std::size_t>(osc::experimental::ShaderType::TOTAL)> g_CameraProjectionStrings =
    {
        "Perspective",
        "Orthographic",
    };
}


// helpers
namespace
{
    template<typename T>
    void DoCopyOnWrite(std::shared_ptr<T>& p)
    {
        if (p.use_count() == 1)
        {
            return;  // sole owner
        }

        p = std::make_shared<T>(*p);
    }

    std::size_t CombineHashes(std::size_t a, std::size_t b)
    {
        return a ^ (b + 0x9e3779b9 + (a<<6) + (a>>2));
    }

    template<typename T>
    std::string StreamToString(T const& v)
    {
        std::stringstream ss;
        ss << v;
        return std::move(ss).str();
    }
}

namespace
{
    enum class IndexFormat {
        UInt16,
        UInt32,
    };

    // represents index data, which may be packed in-memory depending
    // on how it's represented
    union PackedIndex {
        uint32_t u32;
        struct U16Pack { uint16_t a; uint16_t b; } u16;
    };

    static_assert(sizeof(PackedIndex) == sizeof(uint32_t), "double-check that uint32_t is correctly represented on this machine");
    static_assert(alignof(PackedIndex) == alignof(uint32_t), "careful: the union is type-punning between 16- and 32-bit data");

    // pack u16 indices into a u16 `PackedIndex` vector
    void PackAsU16(nonstd::span<uint16_t const> vs, std::vector<PackedIndex>& out)
    {
        if (vs.empty())
        {
            out.clear();
            return;
        }

        out.resize((vs.size()+1)/2);

        std::copy(vs.data(), vs.data() + vs.size(), &out.front().u16.a);

        // zero out the second part of the packed index if there was an odd number
        // of input indices (the 2nd half will be left untouched by the copy above)
        if (vs.size() % 2)
        {
            out.back().u16.b = {};
        }
    }

    void PackAsU16(nonstd::span<uint32_t const> vs, std::vector<PackedIndex>& out)
    {
        if (vs.empty())
        {
            out.clear();
            return;
        }

        out.resize((vs.size()+1)/2);

        // pack two 32-bit source values into one `PackedIndex` per iteration
        for (std::size_t i = 0, end = vs.size()-1; i < end; i += 2)
        {
            PackedIndex& pi = out[i/2];
            pi.u16.a = static_cast<uint16_t>(vs[i]);
            pi.u16.b = static_cast<uint16_t>(vs[i+1]);
        }

        // with an odd number of source values, pack the trailing 32-bit value into
        // the first part of the packed index and zero out the second part
        if (vs.size() % 2)
        {
            PackedIndex& pi = out.back();
            pi.u16.a = static_cast<uint16_t>(vs.back());
            pi.u16.b = {};
        }
    }

    void PackAsU32(nonstd::span<uint16_t const> vs, std::vector<PackedIndex>& out)
    {
        out.clear();
        out.reserve(vs.size());

        for (uint16_t const& v : vs)
        {
            out.push_back(PackedIndex{static_cast<uint32_t>(v)});
        }
    }

    void PackAsU32(nonstd::span<uint32_t const> vs, std::vector<PackedIndex>& out)
    {
        out.clear();
        out.resize(vs.size());
        std::copy(vs.data(), vs.data() + vs.size(), &out.front().u32);
    }

    std::vector<uint32_t> UnpackU32ToU32(nonstd::span<PackedIndex const> data, int numIndices)
    {
        uint32_t const* begin = &data[0].u32;
        uint32_t const* end = begin + numIndices;
        return std::vector<uint32_t>(begin, end);
    }

    std::vector<uint32_t> UnpackU16ToU32(nonstd::span<PackedIndex const> data, int numIndices)
    {
        std::vector<uint32_t> rv;
        rv.reserve(numIndices);

        uint16_t const* it = &data[0].u16.a;
        uint16_t const* end = it + numIndices;
        for (; it != end; ++it)
        {
            rv.push_back(static_cast<uint32_t>(*it));
        }

        return rv;
    }

    std::vector<osc::Rgba32> PackAsRGBA32(nonstd::span<glm::vec4 const> pixels)
    {
        std::vector<osc::Rgba32> rv;
        rv.reserve(pixels.size());
        for (glm::vec4 const& pixel : pixels)
        {
            rv.push_back(osc::Rgba32FromVec4(pixel));
        }
        return rv;
    }

    std::vector<PackedIndex> PackIndexRangeAsU16(std::size_t n)
    {
        OSC_ASSERT(n <= std::numeric_limits<uint16_t>::max());

        std::size_t numEls = (n+1)/2;

        std::vector<PackedIndex> rv;
        rv.reserve(numEls);

        for (std::size_t i = 0, end = n-1; i < end; i += 2)
        {
            PackedIndex& pi = rv.emplace_back();
            pi.u16.a = static_cast<uint16_t>(i);
            pi.u16.b = static_cast<uint16_t>(i+1);
        }

        if (n % 2)
        {
            PackedIndex& pi = rv.emplace_back();
            pi.u16.a = static_cast<uint16_t>(n-1);
            pi.u16.b = {};
        }

        return rv;
    }

    std::vector<PackedIndex> PackIndexRangeAsU32(std::size_t n)
    {
        std::vector<PackedIndex> rv;
        rv.reserve(n);

        for (std::size_t i = 0; i < n; ++i)
        {
            PackedIndex& pi = rv.emplace_back();
            pi.u32 = static_cast<uint32_t>(i);
        }

        return rv;
    }

    template<typename T>
    static std::vector<T> Create0ToNIndices(std::size_t n)
    {
        static_assert(std::is_integral_v<T>);

        OSC_ASSERT(n >= 0);
        OSC_ASSERT(n <= std::numeric_limits<T>::max());

        std::vector<T> rv;
        rv.reserve(n);

        for (T i = 0; i < n; ++i)
        {
            rv.push_back(i);
        }

        return rv;
    }
}


class osc::experimental::Mesh::Impl final {
public:
    Impl() = default;

    Impl(MeshTopography t, nonstd::span<glm::vec3 const> verts) :
        m_Topography{std::move(t)},
        m_Verts(verts.begin(), verts.end()),
        m_IndexFormat{verts.size() <= std::numeric_limits<uint16_t>::max() ? IndexFormat::UInt16 : IndexFormat::UInt32},
        m_NumIndices{static_cast<int>(verts.size())},
        m_IndicesData{m_IndexFormat == IndexFormat::UInt16 ? PackIndexRangeAsU16(verts.size()) : PackIndexRangeAsU32(verts.size())}
    {
        recalculateBounds();
    }

    MeshTopography getTopography() const
    {
        return m_Topography;
    }

    void setTopography(MeshTopography mt)
    {
        m_Topography = mt;
        m_VersionCounter++;
    }

    nonstd::span<glm::vec3 const> getVerts() const
    {
        return m_Verts;
    }

    void setVerts(nonstd::span<const glm::vec3> vs)
    {
        m_Verts.clear();
        m_Verts.reserve(vs.size());

        for (glm::vec3 const& v : vs)
        {
            m_Verts.push_back(v);
        }

        m_GpuBuffersUpToDate = false;
        m_VersionCounter++;
        recalculateBounds();
    }

    nonstd::span<glm::vec3 const> getNormals() const
    {
        return m_Normals;
    }

    void setNormals(nonstd::span<const glm::vec3> ns)
    {
        m_Normals.clear();
        m_Normals.reserve(ns.size());

        for (glm::vec3 const& v : ns)
        {
            m_Normals.push_back(v);
        }

        m_GpuBuffersUpToDate = false;
        m_VersionCounter++;
    }

    nonstd::span<glm::vec2 const> getTexCoords() const
    {
        return m_TexCoords;
    }

    void setTexCoords(nonstd::span<const glm::vec2> tc)
    {
        m_TexCoords.clear();
        m_TexCoords.reserve(tc.size());

        for (glm::vec2 const& t : tc)
        {
            m_TexCoords.push_back(t);
        }

        m_GpuBuffersUpToDate = false;
        m_VersionCounter++;
    }

    void scaleTexCoords(float factor)
    {
        for (glm::vec2& tc : m_TexCoords)
        {
            tc *= factor;
        }

        m_GpuBuffersUpToDate = false;
        m_VersionCounter++;
    }

    int getNumIndices() const
    {
        return m_NumIndices;
    }

    std::vector<uint32_t> getIndices() const
    {
        if (m_IndicesData.empty())
        {
            return std::vector<uint32_t>{};
        }
        else if (m_IndexFormat == IndexFormat::UInt32)
        {
            return UnpackU32ToU32(m_IndicesData, m_NumIndices);
        }
        else
        {
            return UnpackU16ToU32(m_IndicesData, m_NumIndices);
        }
    }

    template<typename T>
    void setIndices(nonstd::span<T const> vs)
    {
        if (m_IndexFormat == IndexFormat::UInt16)
        {
            PackAsU16(vs, m_IndicesData);
        }
        else
        {
            PackAsU32(vs, m_IndicesData);
        }

        m_NumIndices = static_cast<int>(vs.size());
        m_GpuBuffersUpToDate = false;
        m_VersionCounter++;
        recalculateBounds();
    }

    AABB const& getBounds() const
    {
        return m_AABB;
    }

    void clear()
    {
        m_Verts.clear();
        m_Normals.clear();
        m_TexCoords.clear();
        m_NumIndices = 0;
        m_IndicesData.clear();
        m_AABB = AABB{};
        m_TriangleBVH.clear();
        // don't reset version counter (over-hashing is fine, under-hashing is not)
        m_GpuBuffersUpToDate = false;
    }

    void recalculateBounds()
    {
        m_AABB = AABBFromVerts(m_Verts.data(), m_Verts.size());

        if (m_Topography == MeshTopography::Triangles)
        {
            BVH_BuildFromTriangles(m_TriangleBVH, m_Verts.data(), m_Verts.size());
        }
        else
        {
            m_TriangleBVH.clear();
        }
    }

    std::size_t getHash() const
    {
        std::size_t idHash = std::hash<UID>{}(*m_ID);
        std::size_t versionHash = std::hash<int>{}(m_VersionCounter);
        return CombineHashes(idHash, versionHash);
    }

    RayCollision getClosestRayTriangleCollisionModelspace(Line const& modelspaceLine) const
    {
        if (m_Topography != MeshTopography::Triangles)
        {
            return RayCollision{false, 0.0f};
        }

        BVHCollision coll;
        bool collided = BVH_GetClosestRayTriangleCollision(m_TriangleBVH, m_Verts.data(), m_Verts.size(), modelspaceLine, &coll);

        if (collided)
        {
            return RayCollision{true, coll.distance};
        }
        else
        {
            return RayCollision{false, 0.0f};
        }
    }

    RayCollision getClosestRayTriangleCollisionWorldspace(Line const& worldspaceLine, glm::mat4 const& model2world) const
    {
        // do a fast ray-to-AABB collision test
        AABB modelspaceAABB = getBounds();
        AABB worldspaceAABB = AABBApplyXform(modelspaceAABB, model2world);

        RayCollision rayAABBCollision = GetRayCollisionAABB(worldspaceLine, worldspaceAABB);

        if (!rayAABBCollision.hit)
        {
            return rayAABBCollision;  // missed the AABB, so *definitely* missed the mesh
        }

        // it hit the AABB, so it *may* have hit a triangle in the mesh
        //
        // refine the hittest by doing a slower ray-to-triangle test
        glm::mat4 world2model = glm::inverse(model2world);
        Line modelspaceLine = LineApplyXform(worldspaceLine, world2model);

        return getClosestRayTriangleCollisionModelspace(modelspaceLine);
    }

private:
    DefaultConstructOnCopy<UID> m_ID;
    MeshTopography m_Topography = MeshTopography::Triangles;
    std::vector<glm::vec3> m_Verts;
    std::vector<glm::vec3> m_Normals;
    std::vector<glm::vec2> m_TexCoords;
    IndexFormat m_IndexFormat = IndexFormat::UInt16;
    int m_NumIndices = 0;
    std::vector<PackedIndex> m_IndicesData;
    AABB m_AABB = {};
    BVH m_TriangleBVH;
    int m_VersionCounter = 0;
    DefaultConstructOnCopy<bool> m_GpuBuffersUpToDate = false;

    // TODO: GPU data
};


class osc::experimental::Texture2D::Impl final {
public:
    Impl(int width, int height, nonstd::span<Rgba32 const> pixels) :
        m_Dims{width, height},
        m_PixelData{pixels.begin(), pixels.end()}
    {
        OSC_ASSERT(static_cast<int>(m_PixelData.size()) == width*height);
    }

    Impl(int width, int height, nonstd::span<glm::vec4 const> pixels) :
        m_Dims{width, height},
        m_PixelData{PackAsRGBA32(pixels)}
    {
        OSC_ASSERT(static_cast<int>(m_PixelData.size()) == width*height);
    }

    int getWidth() const
    {
        return m_Dims.x;
    }

    int getHeight() const
    {
        return m_Dims.y;
    }

    float getAspectRatio() const
    {
        return VecAspectRatio(m_Dims);
    }

    TextureWrapMode getWrapMode() const
    {
        return getWrapModeU();
    }

    void setWrapMode(TextureWrapMode wm)
    {
        setWrapModeU(wm);
    }

    TextureWrapMode getWrapModeU() const
    {
        return m_WrapModeU;
    }

    void setWrapModeU(TextureWrapMode wm)
    {
        m_WrapModeU = wm;
        m_VersionCounter++;
    }

    TextureWrapMode getWrapModeV() const
    {
        return m_WrapModeV;
    }

    void setWrapModeV(TextureWrapMode wm)
    {
        m_WrapModeV = wm;
        m_VersionCounter++;
    }

    TextureWrapMode getWrapModeW() const
    {
        return m_WrapModeW;
    }

    void setWrapModeW(TextureWrapMode wm)
    {
        m_WrapModeW = wm;
        m_VersionCounter++;
    }

    TextureFilterMode getFilterMode() const
    {
        return m_FilterMode;
    }

    void setFilterMode(TextureFilterMode fm)
    {
        m_FilterMode = fm;
        m_VersionCounter++;
    }

    std::size_t getHash() const
    {
        std::size_t idHash = std::hash<UID>{}(*m_ID);
        std::size_t versionHash = std::hash<int>{}(m_VersionCounter);
        return CombineHashes(idHash, versionHash);
    }

private:
    DefaultConstructOnCopy<UID> m_ID;
    glm::ivec2 m_Dims;
    std::vector<osc::Rgba32> m_PixelData;
    TextureWrapMode m_WrapModeU = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeV = TextureWrapMode::Repeat;
    TextureWrapMode m_WrapModeW = TextureWrapMode::Repeat;
    TextureFilterMode m_FilterMode = TextureFilterMode::Linear;
    int m_VersionCounter = 0;
    DefaultConstructOnCopy<bool> m_GpuBuffersUpToDate = false;

    // TODO: GPU data
};


class osc::experimental::Shader::Impl final {
public:
    explicit Impl(char const*)
    {
        OSC_ASSERT(false && "not yet implemented");
    }

    std::string const& getName() const
    {
        OSC_THROW_NYI();
    }

    std::optional<int> findPropertyIndex(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    std::optional<int> findPropertyIndex(UID) const
    {
        OSC_THROW_NYI();
    }

    int getPropertyCount() const
    {
        OSC_THROW_NYI();
    }

    std::string const& getPropertyName(int) const
    {
        OSC_THROW_NYI();
    }

    UID getPropertyNameID(int) const
    {
        OSC_THROW_NYI();
    }

    ShaderType getPropertyType(int) const
    {
        OSC_THROW_NYI();
    }

    std::size_t getHash() const
    {
        OSC_THROW_NYI();
    }

private:
    // TODO: needs gl::Program
    // TODO: needs definition of where the attrs are
};


class osc::experimental::Material::Impl final {
public:
    Impl(Shader shader) : m_Shader{std::move(shader)}
    {
    }

    Shader const& getShader() const
    {
        OSC_THROW_NYI();
    }

    bool hasProperty(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    bool hasProperty(UID) const
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const* getColor() const
    {
        OSC_THROW_NYI();
    }

    void setColor(glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    float const* getFloat(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    float const* getFloat(UID) const
    {
        OSC_THROW_NYI();
    }

    void setFloat(std::string_view, float)
    {
        OSC_THROW_NYI();
    }

    void setFloat(UID, float)
    {
        OSC_THROW_NYI();
    }

    int const* getInt(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    int const* getInt(UID) const
    {
        OSC_THROW_NYI();
    }

    void setInt(std::string_view, int)
    {
        OSC_THROW_NYI();
    }

    void setInt(UID, int)
    {
        OSC_THROW_NYI();
    }

    Texture2D const* getTexture(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    Texture2D const* getTexture(UID) const
    {
        OSC_THROW_NYI();
    }

    void setTexture(std::string_view, Texture2D const&)
    {
        OSC_THROW_NYI();
    }

    void setTexture(UID, Texture2D const&)
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const* getVector(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const* getVector(UID) const
    {
        OSC_THROW_NYI();
    }

    void setVector(std::string_view, glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    void setVector(UID, glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    glm::mat4 const* getMatrix(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    glm::mat4 const* getMatrix(UID) const
    {
        OSC_THROW_NYI();
    }

    void setMatrix(std::string_view, glm::mat4 const&)
    {
        OSC_THROW_NYI();
    }

    void setMatrix(UID, glm::mat4 const&)
    {
        OSC_THROW_NYI();
    }

    std::size_t getHash() const
    {
        OSC_THROW_NYI();
    }

private:
    Shader m_Shader;
};


class osc::experimental::MaterialPropertyBlock::Impl final {
public:
    void clear()
    {
        OSC_THROW_NYI();
    }

    bool isEmpty() const
    {
        OSC_THROW_NYI();
    }

    bool hasProperty(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    bool hasProperty(UID) const
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const* getColor() const
    {
        OSC_THROW_NYI();
    }

    void setColor(glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    float const* getFloat(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    float const* getFloat(UID) const
    {
        OSC_THROW_NYI();
    }

    void setFloat(std::string_view, float)
    {
        OSC_THROW_NYI();
    }

    void setFloat(UID, float)
    {
        OSC_THROW_NYI();
    }

    int const* getInt(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    int const* getInt(UID) const
    {
        OSC_THROW_NYI();
    }

    void setInt(std::string_view, int)
    {
        OSC_THROW_NYI();
    }

    void setInt(UID, int)
    {
        OSC_THROW_NYI();
    }

    Texture2D const* getTexture(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    Texture2D const* getTexture(UID) const
    {
        OSC_THROW_NYI();
    }

    void setTexture(std::string_view, Texture2D const&)
    {
        OSC_THROW_NYI();
    }

    void setTexture(UID, Texture2D const&)
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const* getVector(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const* getVector(UID) const
    {
        OSC_THROW_NYI();
    }

    void setVector(std::string_view, glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    void setVector(UID, glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    glm::mat4 const* getMatrix(std::string_view) const
    {
        OSC_THROW_NYI();
    }

    glm::mat4 const* getMatrix(UID) const
    {
        OSC_THROW_NYI();
    }

    void setMatrix(std::string_view, glm::mat4 const&)
    {
        OSC_THROW_NYI();
    }

    void setMatrix(UID, glm::mat4 const&)
    {
        OSC_THROW_NYI();
    }

    std::size_t getHash() const
    {
        OSC_THROW_NYI();
    }
private:
    // TODO
};


class osc::experimental::Camera::Impl final {
public:
    explicit Impl(Texture2D)
    {
        OSC_THROW_NYI();
    }

    glm::vec4 const& getBackgroundColor() const
    {
        OSC_THROW_NYI();
    }

    void setBackgroundColor(glm::vec4 const&)
    {
        OSC_THROW_NYI();
    }

    CameraProjection getCameraProjection() const
    {
        OSC_THROW_NYI();
    }

    void setCameraProjection(CameraProjection)
    {
        OSC_THROW_NYI();
    }

    float getOrthographicSize() const
    {
        OSC_THROW_NYI();
    }

    void setOrthographicSize(float)
    {
        OSC_THROW_NYI();
    }

    float getCameraFOV() const
    {
        OSC_THROW_NYI();
    }

    void setCameraFOV(float)
    {
        OSC_THROW_NYI();
    }

    float getNearClippingPlane() const
    {
        OSC_THROW_NYI();
    }

    void setNearClippingPlane(float)
    {
        OSC_THROW_NYI();
    }

    float getFarClippingPlane() const
    {
        OSC_THROW_NYI();
    }

    void setFarClippingPlane(float)
    {
        OSC_THROW_NYI();
    }

    Texture2D const* getTexture() const
    {
        OSC_THROW_NYI();
    }

    void setTexture(Texture2D const&)
    {
        OSC_THROW_NYI();
    }

    void setTexture()
    {
        OSC_THROW_NYI();
    }

    Rect const& getPixelRect() const
    {
        OSC_THROW_NYI();
    }

    void setPixelRect(Rect const&)
    {
        OSC_THROW_NYI();
    }

    int getPixelWidth() const
    {
        OSC_THROW_NYI();
    }

    int getPixelHeight() const
    {
        OSC_THROW_NYI();
    }

    float getAspectRatio() const
    {
        OSC_THROW_NYI();
    }

    std::optional<Rect> getScissorRect() const
    {
        OSC_THROW_NYI();
    }

    void setScissorRect(Rect const&)
    {
        OSC_THROW_NYI();
    }

    void setScissorRect()
    {
        OSC_THROW_NYI();
    }

    glm::vec3 const& getPosition() const
    {
        OSC_THROW_NYI();
    }

    void setPosition(glm::vec3 const&)
    {
        OSC_THROW_NYI();
    }

    glm::vec3 const& getDirection() const
    {
        OSC_THROW_NYI();
    }

    void setDirection(glm::vec3 const&)
    {
        OSC_THROW_NYI();
    }

    glm::mat4 const& getCameraToWorldMatrix() const
    {
        OSC_THROW_NYI();
    }

    void render()
    {
        OSC_THROW_NYI();
    }

    std::size_t getHash() const
    {
        OSC_THROW_NYI();
    }

private:
    // TODO
};


// PUBLIC API

// osc::MeshTopography2

std::ostream& osc::experimental::operator<<(std::ostream& o, MeshTopography t)
{
    return o << g_MeshTopographyStrings.at(static_cast<std::size_t>(t));
}

std::string osc::experimental::to_string(MeshTopography t)
{
    return std::string{g_MeshTopographyStrings.at(static_cast<std::size_t>(t))};
}


// osc::Mesh

osc::experimental::Mesh::Mesh() :
    m_Impl{std::make_shared<Impl>()}
{
}

osc::experimental::Mesh::Mesh(MeshTopography t, nonstd::span<glm::vec3 const> verts) :
    m_Impl{std::make_shared<Impl>(std::move(t), std::move(verts))}
{
}

osc::experimental::Mesh::Mesh(Mesh const&) = default;
osc::experimental::Mesh::Mesh(Mesh&&) noexcept = default;
osc::experimental::Mesh& osc::experimental::Mesh::operator=(Mesh const&) = default;
osc::experimental::Mesh& osc::experimental::Mesh::operator=(Mesh&&) noexcept = default;
osc::experimental::Mesh::~Mesh() noexcept = default;

osc::experimental::MeshTopography osc::experimental::Mesh::getTopography() const
{
    return m_Impl->getTopography();
}

void osc::experimental::Mesh::setTopography(MeshTopography mt)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTopography(std::move(mt));
}

nonstd::span<glm::vec3 const> osc::experimental::Mesh::getVerts() const
{
    return m_Impl->getVerts();
}

void osc::experimental::Mesh::setVerts(nonstd::span<const glm::vec3> vs)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVerts(std::move(vs));
}

nonstd::span<glm::vec3 const> osc::experimental::Mesh::getNormals() const
{
    return m_Impl->getNormals();
}

void osc::experimental::Mesh::setNormals(nonstd::span<const glm::vec3> vs)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setNormals(std::move(vs));
}

nonstd::span<glm::vec2 const> osc::experimental::Mesh::getTexCoords() const
{
    return m_Impl->getTexCoords();
}

void osc::experimental::Mesh::setTexCoords(nonstd::span<const glm::vec2> tc)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexCoords(std::move(tc));
}

void osc::experimental::Mesh::scaleTexCoords(float factor)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->scaleTexCoords(std::move(factor));
}

int osc::experimental::Mesh::getNumIndices() const
{
    return m_Impl->getNumIndices();
}

std::vector<uint32_t> osc::experimental::Mesh::getIndices() const
{
    return m_Impl->getIndices();
}

void osc::experimental::Mesh::setIndices(nonstd::span<const uint16_t> vs)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setIndices(std::move(vs));
}

void osc::experimental::Mesh::setIndices(nonstd::span<const uint32_t> vs)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setIndices(std::move(vs));
}

osc::AABB const& osc::experimental::Mesh::getBounds() const
{
    return m_Impl->getBounds();
}

osc::RayCollision osc::experimental::Mesh::getClosestRayTriangleCollisionModelspace(Line const& modelspaceLine) const
{
    return m_Impl->getClosestRayTriangleCollisionModelspace(modelspaceLine);
}

osc::RayCollision osc::experimental::Mesh::getClosestRayTriangleCollisionWorldspace(Line const& worldspaceLine, glm::mat4 const& model2world) const
{
    return m_Impl->getClosestRayTriangleCollisionWorldspace(worldspaceLine, model2world);
}

void osc::experimental::Mesh::clear()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->clear();
}

bool osc::experimental::operator==(Mesh const& a, Mesh const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(Mesh const& a, Mesh const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(Mesh const& a, Mesh const& b)
{
    return a.m_Impl < b.m_Impl;
}

bool osc::experimental::operator<=(Mesh const& a, Mesh const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Mesh const& a, Mesh const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Mesh const& a, Mesh const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Mesh const& m)
{
    return o << "Mesh(nverts = " << m.m_Impl->getVerts().size() << ", nindices = " << m.m_Impl->getIndices().size() << ')';
}

std::string osc::experimental::to_string(Mesh const& m)
{
    return StreamToString(m);
}

std::size_t std::hash<osc::experimental::Mesh>::operator()(osc::experimental::Mesh const& mesh) const
{
    return mesh.m_Impl->getHash();
}


// osc::TextureWrapMode

std::ostream& osc::experimental::operator<<(std::ostream& o, TextureWrapMode wm)
{
    return o << g_TextureWrapModeStrings.at(static_cast<std::size_t>(wm));
}

std::string osc::experimental::to_string(TextureWrapMode wm)
{
    return std::string{g_TextureWrapModeStrings.at(static_cast<std::size_t>(wm))};
}


// osc::TextureFilterMode

std::ostream& osc::experimental::operator<<(std::ostream& o, TextureFilterMode fm)
{
    return o << g_TextureFilterModeStrings.at(static_cast<std::size_t>(fm));
}

std::string osc::experimental::to_string(TextureFilterMode fm)
{
    return std::string{g_TextureFilterModeStrings.at(static_cast<std::size_t>(fm))};
}


// osc::Texture2D

osc::experimental::Texture2D::Texture2D(int width, int height, nonstd::span<Rgba32 const> pixels) :
    m_Impl{std::make_shared<Impl>(std::move(width), std::move(height), std::move(pixels))}
{
}

osc::experimental::Texture2D::Texture2D(int width, int height, nonstd::span<glm::vec4 const> pixels) :
    m_Impl{std::make_shared<Impl>(std::move(width), std::move(height), std::move(pixels))}
{
}

osc::experimental::Texture2D::Texture2D(Texture2D const&) = default;
osc::experimental::Texture2D::Texture2D(Texture2D&&) noexcept = default;
osc::experimental::Texture2D& osc::experimental::Texture2D::operator=(Texture2D const&) = default;
osc::experimental::Texture2D& osc::experimental::Texture2D::operator=(Texture2D&&) noexcept = default;
osc::experimental::Texture2D::~Texture2D() noexcept = default;

int osc::experimental::Texture2D::getWidth() const
{
    return m_Impl->getWidth();
}

int osc::experimental::Texture2D::getHeight() const
{
    return m_Impl->getHeight();
}

float osc::experimental::Texture2D::getAspectRatio() const
{
    return m_Impl->getAspectRatio();
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapMode() const
{
    return m_Impl->getWrapMode();
}

void osc::experimental::Texture2D::setWrapMode(TextureWrapMode wm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapMode(std::move(wm));
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapModeU() const
{
    return m_Impl->getWrapModeU();
}

void osc::experimental::Texture2D::setWrapModeU(TextureWrapMode wm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeU(std::move(wm));
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapModeV() const
{
    return m_Impl->getWrapModeV();
}

void osc::experimental::Texture2D::setWrapModeV(TextureWrapMode wm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeV(std::move(wm));
}

osc::experimental::TextureWrapMode osc::experimental::Texture2D::getWrapModeW() const
{
    return m_Impl->getWrapModeW();
}

void osc::experimental::Texture2D::setWrapModeW(TextureWrapMode wm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setWrapModeW(std::move(wm));
}

osc::experimental::TextureFilterMode osc::experimental::Texture2D::getFilterMode() const
{
    return m_Impl->getFilterMode();
}

void osc::experimental::Texture2D::setFilterMode(TextureFilterMode fm)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFilterMode(std::move(fm));
}

bool osc::experimental::operator==(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl < b.m_Impl;
}

bool osc::experimental::operator<=(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Texture2D const& a, Texture2D const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Texture2D const& t)
{
    return o << "Texture2D(width = " << t.getWidth() << ", height = " << t.getHeight() << ')';
}

std::string osc::experimental::to_string(Texture2D const& t)
{
    return StreamToString(t);
}

std::size_t std::hash<osc::experimental::Texture2D>::operator()(osc::experimental::Texture2D const& t) const
{
    return t.m_Impl->getHash();
}


// osc::ShaderType

std::ostream& osc::experimental::operator<<(std::ostream& o, ShaderType st)
{
    return o << g_ShaderTypeStrings.at(static_cast<std::size_t>(st));
}

std::string osc::experimental::to_string(ShaderType st)
{
    return std::string{g_ShaderTypeStrings.at(static_cast<std::size_t>(st))};
}


// shader IDs

osc::UID osc::experimental::StorePropertyNameToUID(std::string_view)
{
    throw std::runtime_error{"todo"};
}

std::optional<std::string_view> osc::experimental::TryLoadPropertyNameFromUID(UID)
{
    throw std::runtime_error{"todo"};
}


// osc::Shader

osc::experimental::Shader::Shader(char const* src) :
    m_Impl{std::make_shared<Impl>(src)}
{
}

osc::experimental::Shader::Shader(Shader const&) = default;
osc::experimental::Shader::Shader(Shader&&) noexcept = default;
osc::experimental::Shader& osc::experimental::Shader::operator=(Shader const&) = default;
osc::experimental::Shader& osc::experimental::Shader::operator=(Shader&&) noexcept = default;
osc::experimental::Shader::~Shader() noexcept = default;

std::string const& osc::experimental::Shader::getName() const
{
    return m_Impl->getName();
}

std::optional<int> osc::experimental::Shader::findPropertyIndex(std::string_view propertyName) const
{
    return m_Impl->findPropertyIndex(std::move(propertyName));
}

std::optional<int> osc::experimental::Shader::findPropertyIndex(UID propertyNameID) const
{
    return m_Impl->findPropertyIndex(std::move(propertyNameID));
}

int osc::experimental::Shader::getPropertyCount() const
{
    return m_Impl->getPropertyCount();
}

std::string const& osc::experimental::Shader::getPropertyName(int propertyIndex) const
{
    return m_Impl->getPropertyName(std::move(propertyIndex));
}

osc::UID osc::experimental::Shader::getPropertyNameID(int propertyIndex) const
{
    return m_Impl->getPropertyNameID(std::move(propertyIndex));
}

osc::experimental::ShaderType osc::experimental::Shader::getPropertyType(int propertyIndex) const
{
    return m_Impl->getPropertyType(std::move(propertyIndex));
}

bool osc::experimental::operator==(Shader const& a, Shader const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(Shader const& a, Shader const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(Shader const& a, Shader const& b)
{
    return a.m_Impl < b.m_Impl;
}

bool osc::experimental::operator<=(Shader const& a, Shader const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Shader const& a, Shader const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Shader const& a, Shader const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Shader const& s)
{
    return o << "Shader(name = " << s.getName() << ')';
}

std::string osc::experimental::to_string(Shader const& s)
{
    return StreamToString(s);
}

std::size_t std::hash<osc::experimental::Shader>::operator()(osc::experimental::Shader const& s) const
{
    return s.m_Impl->getHash();
}


// osc::Material

osc::experimental::Material::Material(Shader shader) :
    m_Impl{std::make_shared<Impl>(std::move(shader))}
{
}

osc::experimental::Material::Material(Material const&) = default;
osc::experimental::Material::Material(Material&&) noexcept = default;
osc::experimental::Material& osc::experimental::Material::operator=(Material const&) = default;
osc::experimental::Material& osc::experimental::Material::operator=(Material&&) noexcept = default;
osc::experimental::Material::~Material() noexcept = default;

osc::experimental::Shader const& osc::experimental::Material::getShader() const
{
    return m_Impl->getShader();
}

bool osc::experimental::Material::hasProperty(std::string_view propertyName) const
{
    return m_Impl->hasProperty(std::move(propertyName));
}

bool osc::experimental::Material::hasProperty(UID propertyNameID) const
{
    return m_Impl->hasProperty(std::move(propertyNameID));
}

glm::vec4 const* osc::experimental::Material::getColor() const
{
    return m_Impl->getVector("Color");
}

void osc::experimental::Material::setColor(glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVector("Color", v);
}

float const* osc::experimental::Material::getFloat(std::string_view propertyName) const
{
    return m_Impl->getFloat(std::move(propertyName));
}

float const* osc::experimental::Material::getFloat(UID propertyNameID) const
{
    return m_Impl->getFloat(std::move(propertyNameID));
}

void osc::experimental::Material::setFloat(std::string_view propertyName, float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloat(std::move(propertyName), std::move(v));
}

void osc::experimental::Material::setFloat(UID propertyNameID, float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloat(std::move(propertyNameID), std::move(v));
}

int const* osc::experimental::Material::getInt(std::string_view propertyName) const
{
    return m_Impl->getInt(std::move(propertyName));
}

int const* osc::experimental::Material::getInt(UID propertyNameID) const
{
    return m_Impl->getInt(std::move(propertyNameID));
}

void osc::experimental::Material::setInt(std::string_view propertyName, int v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setInt(std::move(propertyName), std::move(v));
}

void osc::experimental::Material::setInt(UID propertyNameID, int v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setInt(std::move(propertyNameID), std::move(v));
}

osc::experimental::Texture2D const* osc::experimental::Material::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(std::move(propertyName));
}

osc::experimental::Texture2D const* osc::experimental::Material::getTexture(UID propertyNameID) const
{
    return m_Impl->getTexture(std::move(propertyNameID));
}

void osc::experimental::Material::setTexture(std::string_view propertyName, Texture2D const& t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(propertyName), std::move(t));
}

void osc::experimental::Material::setTexture(UID propertyNameID, Texture2D const& t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(propertyNameID), std::move(t));
}

glm::vec4 const* osc::experimental::Material::getVector(std::string_view propertyName) const
{
    return m_Impl->getVector(std::move(propertyName));
}

glm::vec4 const* osc::experimental::Material::getVector(UID propertyNameID) const
{
    return m_Impl->getVector(std::move(propertyNameID));
}

void osc::experimental::Material::setVector(std::string_view propertyName, glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVector(std::move(propertyName), v);
}

void osc::experimental::Material::setVector(UID propertyNameID, glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVector(std::move(propertyNameID), v);
}

glm::mat4 const* osc::experimental::Material::getMatrix(std::string_view propertyName) const
{
    return m_Impl->getMatrix(std::move(propertyName));
}

glm::mat4 const* osc::experimental::Material::getMatrix(UID propertyNameID) const
{
    return m_Impl->getMatrix(std::move(propertyNameID));
}

void osc::experimental::Material::setMatrix(std::string_view propertyName, glm::mat4 const& m)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMatrix(std::move(propertyName), m);
}

void osc::experimental::Material::setMatrix(UID propertyNameID, glm::mat4 const& m)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMatrix(std::move(propertyNameID), m);
}

bool osc::experimental::operator==(Material const& a, Material const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(Material const& a, Material const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(Material const& a, Material const& b)
{
    return a.m_Impl < b.m_Impl;
}

bool osc::experimental::operator<=(Material const& a, Material const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Material const& a, Material const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Material const& a, Material const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Material const&)
{
    return o << "Material()";
}

std::string osc::experimental::to_string(Material const& mat)
{
    return StreamToString(mat);
}

std::size_t std::hash<osc::experimental::Material>::operator()(osc::experimental::Material const& mat) const
{
    return mat.m_Impl->getHash();
}


// osc::MaterialPropertyBlock

osc::experimental::MaterialPropertyBlock::MaterialPropertyBlock() : m_Impl{std::make_shared<Impl>()}
{
}

osc::experimental::MaterialPropertyBlock::MaterialPropertyBlock(MaterialPropertyBlock const&) = default;
osc::experimental::MaterialPropertyBlock::MaterialPropertyBlock(MaterialPropertyBlock&&) noexcept = default;
osc::experimental::MaterialPropertyBlock& osc::experimental::MaterialPropertyBlock::operator=(MaterialPropertyBlock const&) = default;
osc::experimental::MaterialPropertyBlock& osc::experimental::MaterialPropertyBlock::operator=(MaterialPropertyBlock&&) noexcept = default;
osc::experimental::MaterialPropertyBlock::~MaterialPropertyBlock() noexcept = default;

void osc::experimental::MaterialPropertyBlock::clear()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->clear();
}

bool osc::experimental::MaterialPropertyBlock::isEmpty() const
{
    return m_Impl->isEmpty();
}

bool osc::experimental::MaterialPropertyBlock::hasProperty(std::string_view propertyName) const
{
    return m_Impl->hasProperty(std::move(propertyName));
}

bool osc::experimental::MaterialPropertyBlock::hasProperty(UID propertyNameID) const
{
    return m_Impl->hasProperty(std::move(propertyNameID));
}

glm::vec4 const* osc::experimental::MaterialPropertyBlock::getColor() const
{
    return m_Impl->getVector("Color");
}

void osc::experimental::MaterialPropertyBlock::setColor(glm::vec4 const& c)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVector("Color", c);
}

float const* osc::experimental::MaterialPropertyBlock::getFloat(std::string_view propertyName) const
{
    return m_Impl->getFloat(std::move(propertyName));
}

float const* osc::experimental::MaterialPropertyBlock::getFloat(UID propertyNameID) const
{
    return m_Impl->getFloat(std::move(propertyNameID));
}

void osc::experimental::MaterialPropertyBlock::setFloat(std::string_view propertyName, float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloat(std::move(propertyName), std::move(v));
}

void osc::experimental::MaterialPropertyBlock::setFloat(UID propertyNameID, float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFloat(std::move(propertyNameID), std::move(v));
}

int const* osc::experimental::MaterialPropertyBlock::getInt(std::string_view propertyName) const
{
    return m_Impl->getInt(std::move(propertyName));
}

int const* osc::experimental::MaterialPropertyBlock::getInt(UID propertyNameID) const
{
    return m_Impl->getInt(std::move(propertyNameID));
}

void osc::experimental::MaterialPropertyBlock::setInt(std::string_view propertyName, int v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setInt(std::move(propertyName), std::move(v));
}

void osc::experimental::MaterialPropertyBlock::setInt(UID propertyNameID, int v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setInt(std::move(propertyNameID), std::move(v));
}

osc::experimental::Texture2D const* osc::experimental::MaterialPropertyBlock::getTexture(std::string_view propertyName) const
{
    return m_Impl->getTexture(std::move(propertyName));
}

osc::experimental::Texture2D const* osc::experimental::MaterialPropertyBlock::getTexture(UID propertyNameID) const
{
    return m_Impl->getTexture(std::move(propertyNameID));
}

void osc::experimental::MaterialPropertyBlock::setTexture(std::string_view propertyName, Texture2D const& t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(propertyName), std::move(t));
}

void osc::experimental::MaterialPropertyBlock::setTexture(UID propertyNameID, Texture2D const& t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(propertyNameID), std::move(t));
}

glm::vec4 const* osc::experimental::MaterialPropertyBlock::getVector(std::string_view propertyName) const
{
    return m_Impl->getVector(std::move(propertyName));
}

glm::vec4 const* osc::experimental::MaterialPropertyBlock::getVector(UID propertyNameID) const
{
    return m_Impl->getVector(std::move(propertyNameID));
}

void osc::experimental::MaterialPropertyBlock::setVector(std::string_view propertyName, glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVector(propertyName, v);
}

void osc::experimental::MaterialPropertyBlock::setVector(UID propertyNameID, glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setVector(std::move(propertyNameID), v);
}

glm::mat4 const* osc::experimental::MaterialPropertyBlock::getMatrix(std::string_view propertyName) const
{
    return m_Impl->getMatrix(std::move(propertyName));
}

glm::mat4 const* osc::experimental::MaterialPropertyBlock::getMatrix(UID propertyNameID) const
{
    return m_Impl->getMatrix(std::move(propertyNameID));
}

void osc::experimental::MaterialPropertyBlock::setMatrix(std::string_view propertyName, glm::mat4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMatrix(std::move(propertyName), v);
}

void osc::experimental::MaterialPropertyBlock::setMatrix(UID propertyNameID, glm::mat4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setMatrix(std::move(propertyNameID), v);
}

bool osc::experimental::operator==(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl < b.m_Impl;
}

bool osc::experimental::operator<=(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(MaterialPropertyBlock const& a, MaterialPropertyBlock const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, MaterialPropertyBlock const&)
{
    return o << "MaterialPropertyBlock()";
}

std::string osc::experimental::to_string(MaterialPropertyBlock const& m)
{
    return StreamToString(m);
}

std::size_t std::hash<osc::experimental::MaterialPropertyBlock>::operator()(osc::experimental::MaterialPropertyBlock const& m) const
{
    return m.m_Impl->getHash();
}


// osc::CameraProjection

std::ostream& osc::experimental::operator<<(std::ostream& o, CameraProjection p)
{
    return o << g_CameraProjectionStrings.at(static_cast<std::size_t>(p));
}

std::string osc::experimental::to_string(CameraProjection p)
{
    return std::string{g_CameraProjectionStrings.at(static_cast<std::size_t>(p))};
}


// osc::CameraNew

osc::experimental::Camera::Camera(Texture2D t) :
    m_Impl{std::make_shared<Impl>(std::move(t))}
{
}

osc::experimental::Camera::Camera(Camera const&) = default;
osc::experimental::Camera::Camera(Camera&&) noexcept = default;
osc::experimental::Camera& osc::experimental::Camera::operator=(Camera const&) = default;
osc::experimental::Camera& osc::experimental::Camera::operator=(Camera&&) noexcept = default;
osc::experimental::Camera::~Camera() noexcept = default;

glm::vec4 const& osc::experimental::Camera::getBackgroundColor() const
{
    return m_Impl->getBackgroundColor();
}

void osc::experimental::Camera::setBackgroundColor(glm::vec4 const& v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setBackgroundColor(v);
}

osc::experimental::CameraProjection osc::experimental::Camera::getCameraProjection() const
{
    return m_Impl->getCameraProjection();
}

void osc::experimental::Camera::setCameraProjection(CameraProjection p)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setCameraProjection(std::move(p));
}

float osc::experimental::Camera::getOrthographicSize() const
{
    return m_Impl->getOrthographicSize();
}

void osc::experimental::Camera::setOrthographicSize(float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setOrthographicSize(std::move(v));
}

float osc::experimental::Camera::getCameraFOV() const
{
    return m_Impl->getCameraFOV();
}

void osc::experimental::Camera::setCameraFOV(float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setCameraFOV(std::move(v));
}

float osc::experimental::Camera::getNearClippingPlane() const
{
    return m_Impl->getNearClippingPlane();
}

void osc::experimental::Camera::setNearClippingPlane(float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setNearClippingPlane(std::move(v));
}

float osc::experimental::Camera::getFarClippingPlane() const
{
    return m_Impl->getFarClippingPlane();
}

void osc::experimental::Camera::setFarClippingPlane(float v)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setFarClippingPlane(std::move(v));
}

osc::experimental::Texture2D const* osc::experimental::Camera::getTexture() const
{
    return m_Impl->getTexture();
}

void osc::experimental::Camera::setTexture(Texture2D const& t)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture(std::move(t));
}

void osc::experimental::Camera::setTexture()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setTexture();
}

int osc::experimental::Camera::getPixelWidth() const
{
    return m_Impl->getPixelWidth();
}

int osc::experimental::Camera::getPixelHeight() const
{
    return m_Impl->getPixelHeight();
}

float osc::experimental::Camera::getAspectRatio() const
{
    return m_Impl->getAspectRatio();
}

std::optional<osc::Rect> osc::experimental::Camera::getScissorRect() const
{
    return m_Impl->getScissorRect();
}

void osc::experimental::Camera::setScissorRect(Rect const& r)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setScissorRect(r);
}

void osc::experimental::Camera::setScissorRect()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setScissorRect();
}

glm::vec3 const& osc::experimental::Camera::getPosition() const
{
    return m_Impl->getPosition();
}

void osc::experimental::Camera::setPosition(glm::vec3 const& p)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setPosition(p);
}

glm::vec3 const& osc::experimental::Camera::getDirection() const
{
    return m_Impl->getDirection();
}

void osc::experimental::Camera::setDirection(glm::vec3 const& d)
{
    DoCopyOnWrite(m_Impl);
    m_Impl->setDirection(d);
}

glm::mat4 const& osc::experimental::Camera::getCameraToWorldMatrix() const
{
    return m_Impl->getCameraToWorldMatrix();
}

void osc::experimental::Camera::render()
{
    DoCopyOnWrite(m_Impl);
    m_Impl->render();
}

bool osc::experimental::operator==(Camera const& a, Camera const& b)
{
    return a.m_Impl == b.m_Impl;
}

bool osc::experimental::operator!=(Camera const& a, Camera const& b)
{
    return a.m_Impl != b.m_Impl;
}

bool osc::experimental::operator<(Camera const& a, Camera const& b)
{
    return a.m_Impl < b.m_Impl;
}

bool osc::experimental::operator<=(Camera const& a, Camera const& b)
{
    return a.m_Impl <= b.m_Impl;
}

bool osc::experimental::operator>(Camera const& a, Camera const& b)
{
    return a.m_Impl > b.m_Impl;
}

bool osc::experimental::operator>=(Camera const& a, Camera const& b)
{
    return a.m_Impl >= b.m_Impl;
}

std::ostream& osc::experimental::operator<<(std::ostream& o, Camera const&)
{
    return o << "CameraNew()";
}

std::string osc::experimental::to_string(Camera const& c)
{
    return StreamToString(c);
}

size_t std::hash<osc::experimental::Camera>::operator()(osc::experimental::Camera const& c) const
{
    return c.m_Impl->getHash();
}


// osc::GraphicsBackend

void osc::experimental::Graphics::DrawMesh(Mesh,
                                           glm::vec3 const&,
                                           Material,
                                           Camera&,
                                           std::optional<MaterialPropertyBlock>)
{
    // - copy the data onto the camera's command buffer list
    // - deal with any batch flushing etc.
    OSC_THROW_NYI();
}

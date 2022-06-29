#pragma once

#include "src/Maths/Rect.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/mat4x3.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace osc
{
    struct AABB;
    struct Rgba32;
    struct Transform;
}

// 2D texture
//
// encapsulates an image that can be sampled by shaders
namespace osc::experimental
{
    // how texels should be sampled when a texture coordinate falls outside the texture's bounds
    enum class TextureWrapMode {
        Repeat = 0,
        Clamp,
        Mirror,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, TextureWrapMode);
    std::string to_string(TextureWrapMode);

    // how sampling should handle when the sampling location falls between multiple textels
    enum class TextureFilterMode {
        Nearest = 0,
        Linear,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, TextureFilterMode);
    std::string to_string(TextureFilterMode);

    // a handle to a 2D texture that can be rendered by the graphics backend
    class Texture2D final {
    public:
        // RGBA32, SRGB
        Texture2D(int width, int height, nonstd::span<Rgba32 const> pixelsRowByRow);
        Texture2D(Texture2D const&);
        Texture2D(Texture2D&&) noexcept;
        Texture2D& operator=(Texture2D const&);
        Texture2D& operator=(Texture2D&&) noexcept;
        ~Texture2D() noexcept;

        int getWidth() const;
        int getHeight() const;
        float getAspectRatio() const;

        TextureWrapMode getWrapMode() const;  // same as getWrapModeU
        void setWrapMode(TextureWrapMode);
        TextureWrapMode getWrapModeU() const;
        void setWrapModeU(TextureWrapMode);
        TextureWrapMode getWrapModeV() const;
        void setWrapModeV(TextureWrapMode);
        TextureWrapMode getWrapModeW() const;
        void setWrapModeW(TextureWrapMode);

        TextureFilterMode getFilterMode() const;
        void setFilterMode(TextureFilterMode);

        class Impl;
    private:
        friend class GraphicsBackend;
        friend bool operator==(Texture2D const&, Texture2D const&);
        friend bool operator!=(Texture2D const&, Texture2D const&);
        friend bool operator<(Texture2D const&, Texture2D const&);
        friend bool operator<=(Texture2D const&, Texture2D const&);
        friend bool operator>(Texture2D const&, Texture2D const&);
        friend bool operator>=(Texture2D const&, Texture2D const&);
        friend std::ostream& operator<<(std::ostream&, Texture2D const&);
        friend std::string to_string(Texture2D const&);

        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Texture2D const&, Texture2D const&);
    bool operator!=(Texture2D const&, Texture2D const&);
    bool operator<(Texture2D const&, Texture2D const&);
    bool operator<=(Texture2D const&, Texture2D const&);
    bool operator>(Texture2D const&, Texture2D const&);
    bool operator>=(Texture2D const&, Texture2D const&);
    std::ostream& operator<<(std::ostream&, Texture2D const&);
    std::string to_string(Texture2D const&);
}


// shader
//
// encapsulates a shader program that has been compiled + initialized by the backend
namespace osc::experimental
{
    // data type of a material-assignable property parsed from the shader code
    enum class ShaderType {
        Float = 0,
        Vec3,
        Vec4,
        Mat3,
        Mat4,
        Mat4x3,
        Int,
        Bool,
        Sampler2D,
        Unknown,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, ShaderType);
    std::string to_string(ShaderType);

    // a handle to a shader
    class Shader final {
    public:
        Shader(char const* vertexShader, char const* fragmentShader);  // throws on compile error
        Shader(Shader const&);
        Shader(Shader&&) noexcept;
        Shader& operator=(Shader const&);
        Shader& operator=(Shader&&) noexcept;
        ~Shader() noexcept;

        std::optional<int> findPropertyIndex(std::string const& propertyName) const;

        int getPropertyCount() const;
        std::string const& getPropertyName(int propertyIndex) const;
        ShaderType getPropertyType(int propertyIndex) const;

        class Impl;
    private:
        friend class GraphicsBackend;
        friend bool operator==(Shader const&, Shader const&);
        friend bool operator!=(Shader const&, Shader const&);
        friend bool operator<(Shader const&, Shader const&);
        friend bool operator<=(Shader const&, Shader const&);
        friend bool operator>(Shader const&, Shader const&);
        friend bool operator>=(Shader const&, Shader const&);
        friend std::ostream& operator<<(std::ostream&, Shader const&);
        friend std::string to_string(Shader const&);

        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Shader const&, Shader const&);
    bool operator!=(Shader const&, Shader const&);
    bool operator<(Shader const&, Shader const&);
    bool operator<=(Shader const&, Shader const&);
    bool operator>(Shader const&, Shader const&);
    bool operator>=(Shader const&, Shader const&);
    std::ostream& operator<<(std::ostream&, Shader const&);
    std::string to_string(Shader const&);
}


// material
//
// pairs a shader with properties that the shader program is used with
namespace osc::experimental
{
    class Material final {
    public:
        explicit Material(Shader);
        Material(Material const&);
        Material(Material&&) noexcept;
        Material& operator=(Material const&);
        Material& operator=(Material&&) noexcept;
        ~Material() noexcept;

        Shader const& getShader() const;

        std::optional<float> getFloat(std::string_view propertyName) const;
        void setFloat(std::string_view propertyName, float);

        std::optional<glm::vec3> getVec3(std::string_view propertyName) const;
        void setVec3(std::string_view propertyName, glm::vec3);

        std::optional<glm::vec4> getVec4(std::string_view propertyName) const;
        void setVec4(std::string_view propertyName, glm::vec4);

        std::optional<glm::mat3> getMat3(std::string_view propertyName) const;
        void setMat3(std::string_view propertyName, glm::mat3 const&);

        std::optional<glm::mat4> getMat4(std::string_view propertyName) const;
        void setMat4(std::string_view propertyName, glm::mat4 const&);

        std::optional<glm::mat4x3> getMat4x3(std::string_view propertyName) const;
        void setMat4x3(std::string_view propertyName, glm::mat4x3 const&);

        std::optional<int> getInt(std::string_view propertyName) const;
        void setInt(std::string_view, int);

        std::optional<bool> getBool(std::string_view propertyName) const;
        void setBool(std::string_view propertyName, bool);

        std::optional<Texture2D> getTexture(std::string_view propertyName) const;
        void setTexture(std::string_view, Texture2D);

        class Impl;
    private:
        friend class GraphicsBackend;
        friend bool operator==(Material const&, Material const&);
        friend bool operator!=(Material const&, Material const&);
        friend bool operator<(Material const&, Material const&);
        friend bool operator<=(Material const&, Material const&);
        friend bool operator>(Material const&, Material const&);
        friend bool operator>=(Material const&, Material const&);
        friend std::ostream& operator<<(std::ostream&, Material const&);
        friend std::string to_string(Material const&);

        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Material const&, Material const&);
    bool operator!=(Material const&, Material const&);
    bool operator<(Material const&, Material const&);
    bool operator<=(Material const&, Material const&);
    bool operator>(Material const&, Material const&);
    bool operator>=(Material const&, Material const&);
    std::ostream& operator<<(std::ostream&, Material const&);
    std::string to_string(Material const&);
}


// material property block
//
// enables callers to apply per-instance properties when using a material (more efficiently
// than using a different material every time)
namespace osc::experimental
{
    class MaterialPropertyBlock final {
    public:
        MaterialPropertyBlock();
        MaterialPropertyBlock(MaterialPropertyBlock const&);
        MaterialPropertyBlock(MaterialPropertyBlock&&) noexcept;
        MaterialPropertyBlock& operator=(MaterialPropertyBlock const&);
        MaterialPropertyBlock& operator=(MaterialPropertyBlock&&) noexcept;
        ~MaterialPropertyBlock() noexcept;

        void clear();
        bool isEmpty() const;

        std::optional<float> getFloat(std::string_view propertyName) const;
        void setFloat(std::string_view propertyName, float);

        std::optional<glm::vec3> getVec3(std::string_view propertyName) const;
        void setVec3(std::string_view propertyName, glm::vec3);

        std::optional<glm::vec4> getVec4(std::string_view propertyName) const;
        void setVec4(std::string_view propertyName, glm::vec4);

        std::optional<glm::mat3> getMat3(std::string_view propertyName) const;
        void setMat3(std::string_view propertyName, glm::mat3 const&);

        std::optional<glm::mat4> getMat4(std::string_view propertyName) const;
        void setMat4(std::string_view propertyName, glm::mat4 const&);

        std::optional<glm::mat4x3> getMat4x3(std::string_view propertyName) const;
        void setMat4x3(std::string_view propertyName, glm::mat4x3 const&);

        std::optional<int> getInt(std::string_view propertyName) const;
        void setInt(std::string_view, int);

        std::optional<bool> getBool(std::string_view propertyName) const;
        void setBool(std::string_view propertyName, bool);

        std::optional<Texture2D> getTexture(std::string_view propertyName) const;
        void setTexture(std::string_view, Texture2D);

        class Impl;
    private:
        friend class GraphicsBackend;
        friend bool operator==(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend bool operator!=(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend bool operator<(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend bool operator<=(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend bool operator>(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend bool operator>=(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend std::ostream& operator<<(std::ostream&, MaterialPropertyBlock const&);
        friend std::string to_string(MaterialPropertyBlock const&);

        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    bool operator!=(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    bool operator<(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    bool operator<=(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    bool operator>(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    bool operator>=(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    std::ostream& operator<<(std::ostream&, MaterialPropertyBlock const&);
    std::string to_string(MaterialPropertyBlock const&);
}


// mesh
//
// encapsulates mesh data, which may include vertices, indices, normals, texture
// coordinates, etc.
namespace osc::experimental
{
    // which primitive geometry the mesh data represents
    enum class MeshTopography {
        Triangles = 0,
        Lines,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, MeshTopography);
    std::string to_string(MeshTopography);

    class Mesh final {
    public:
        Mesh();
        Mesh(Mesh const&);
        Mesh(Mesh&&) noexcept;
        Mesh& operator=(Mesh const&);
        Mesh& operator=(Mesh&&) noexcept;
        ~Mesh() noexcept;

        MeshTopography getTopography() const;
        void setTopography(MeshTopography);

        nonstd::span<glm::vec3 const> getVerts() const;
        void setVerts(nonstd::span<glm::vec3 const>);

        nonstd::span<glm::vec3 const> getNormals() const;
        void setNormals(nonstd::span<glm::vec3 const>);

        nonstd::span<glm::vec2 const> getTexCoords() const;
        void setTexCoords(nonstd::span<glm::vec2 const>);

        int getNumIndices() const;
        std::vector<std::uint32_t> getIndices() const;
        void setIndices(nonstd::span<std::uint16_t const>);
        void setIndices(nonstd::span<std::uint32_t const>);

        AABB const& getBounds() const;  // local-space

        void clear();

        class Impl;
    private:
        friend class GraphicsBackend;
        friend bool operator==(Mesh const&, Mesh const&);
        friend bool operator!=(Mesh const&, Mesh const&);
        friend bool operator<(Mesh const&, Mesh const&);
        friend bool operator<=(Mesh const&, Mesh const&);
        friend bool operator>(Mesh const&, Mesh const&);
        friend bool operator>=(Mesh const&, Mesh const&);
        friend std::ostream& operator<<(std::ostream&, Mesh const&);
        friend std::string to_string(Mesh const&);

        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Mesh const&, Mesh const&);
    bool operator!=(Mesh const&, Mesh const&);
    bool operator<(Mesh const&, Mesh const&);
    bool operator<=(Mesh const&, Mesh const&);
    bool operator>(Mesh const&, Mesh const&);
    bool operator>=(Mesh const&, Mesh const&);
    std::ostream& operator<<(std::ostream&, Mesh const&);
    std::string to_string(Mesh const&);
}

// render texture
//
// a texture that can be rendered to
namespace osc::experimental
{
    enum class RenderTextureFormat {
        ARGB32 = 0,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, RenderTextureFormat);
    std::string to_string(RenderTextureFormat);

    enum class DepthStencilFormat {
        D24_UNorm_S8_UInt = 0,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, DepthStencilFormat);
    std::string to_string(DepthStencilFormat);

    class RenderTextureDescriptor final {
    public:
        RenderTextureDescriptor(int width, int height);

        int getWidth() const;
        void setWidth(int);

        int getHeight() const;
        void setHeight(int);

        int getAntialiasingLevel() const;
        void setAntialiasingLevel(int);

        int getDepth() const;
        void setDepth(int);  // 0, 16, 24, or 32

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

    private:
        friend bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend bool operator!=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend bool operator<(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend bool operator<=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend bool operator>(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);
        friend std::string to_string(RenderTextureDescriptor const&);

        int m_Width;
        int m_Height;
        int m_AnialiasingLevel;
        int m_Depth;
        RenderTextureFormat m_ColorFormat;
        DepthStencilFormat m_DepthStencilFormat;
    };

    bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    bool operator!=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    bool operator<(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    bool operator<=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    bool operator>(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);
    std::string to_string(RenderTextureDescriptor const&);

    class RenderTexture final {
    public:
        RenderTexture(RenderTextureDescriptor const&);

        int getWidth() const;
        void setWidth(int);

        int getHeight() const;
        void setHeight(int);

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        int getAntialiasingLevel() const;
        void setAntialiasingLevel(int);

        int getDepth() const;
        void setDepth(int);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

        class Impl;
    private:
        friend bool operator==(RenderTexture const&, RenderTexture const&);
        friend bool operator!=(RenderTexture const&, RenderTexture const&);
        friend bool operator<(RenderTexture const&, RenderTexture const&);
        friend bool operator<=(RenderTexture const&, RenderTexture const&);
        friend bool operator>(RenderTexture const&, RenderTexture const&);
        friend std::ostream& operator<<(std::ostream&, RenderTexture const&);
        friend std::string to_string(RenderTexture const&);

        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(RenderTexture const&, RenderTexture const&);
    bool operator!=(RenderTexture const&, RenderTexture const&);
    bool operator<(RenderTexture const&, RenderTexture const&);
    bool operator<=(RenderTexture const&, RenderTexture const&);
    bool operator>(RenderTexture const&, RenderTexture const&);
    std::ostream& operator<<(std::ostream&, RenderTexture const&);
    std::string to_string(RenderTexture const&);
}

// camera
//
// encapsulates a camera viewport that can be drawn to, with the intention of producing
// a 2D rendered image of the drawn elements
namespace osc::experimental
{
    // the shape of the viewing frustrum that the camera uses
    enum class CameraProjection {
        Perspective = 0,
        Orthographic,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, CameraProjection);
    std::string to_string(CameraProjection);

    class Camera final {
    public:
        Camera();  // draws to screen
        explicit Camera(RenderTexture);  // draws to texture
        Camera(Camera const&);
        Camera(Camera&&) noexcept;
        Camera& operator=(Camera const&);
        Camera& operator=(Camera&&) noexcept;
        ~Camera() noexcept;

        glm::vec4 getBackgroundColor() const;
        void setBackgroundColor(glm::vec4 const&);

        CameraProjection getCameraProjection() const;
        void setCameraProjection(CameraProjection);

        // only used if CameraProjection == Orthographic
        float getOrthographicSize() const;
        void setOrthographicSize(float);

        // only used if CameraProjection == Perspective
        float getCameraFOV() const;
        void setCameraFOV(float);

        float getNearClippingPlane() const;
        void setNearClippingPlane(float);

        float getFarClippingPlane() const;
        void setFarClippingPlane(float);

        std::optional<RenderTexture> getTexture() const;  // empty if drawing directly to screen
        void setTexture(RenderTexture);
        void setTexture();  // resets to drawing to screen

        // where on the screen the camera is rendered (in screen-space)
        //
        // returns rect with topleft coord 0,0  and a width/height of texture if drawing
        // to a texture
        Rect getPixelRect() const;
        void setPixelRect(Rect const&);

        int getPixelWidth() const;
        int getPixelHeight() const;
        float getAspectRatio() const;

        // scissor testing
        //
        // This tells the rendering backend to only render the fragments that occur within
        // these bounds. It's useful when (e.g.) running an expensive fragment shader (e.g.
        // image processing kernels) where you know that only a certain subspace is actually
        // interesting (e.g. rim-highlighting only selected elements)
        std::optional<Rect> getScissorRect() const;  // std::nullopt if not scissor testing
        void setScissorRect(Rect const&);  // rect is in pixel space?
        void setScissorRect();  // resets to having no scissor

        glm::vec3 getPosition() const;
        void setPosition(glm::vec3 const&);

        glm::vec3 getDirection() const;
        void setDirection(glm::vec3 const&);

        glm::mat4 getCameraToWorldMatrix() const;

        // flushes any rendering commands that were queued against this camera
        //
        // after this call completes, the output texture, or screen, should contain
        // the rendered geometry
        void render();

        class Impl;
    private:
        friend class GraphicsBackend;
        friend bool operator==(Camera const&, Camera const&);
        friend bool operator!=(Camera const&, Camera const&);
        friend bool operator<(Camera const&, Camera const&);
        friend bool operator<=(Camera const&, Camera const&);
        friend bool operator>(Camera const&, Camera const&);
        friend bool operator>=(Camera const&, Camera const&);
        friend std::ostream& operator<<(std::ostream&, Camera const&);
        friend std::string to_string(Camera const&);

        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Camera const&, Camera const&);
    bool operator!=(Camera const&, Camera const&);
    bool operator<(Camera const&, Camera const&);
    bool operator<=(Camera const&, Camera const&);
    bool operator>(Camera const&, Camera const&);
    bool operator>=(Camera const&, Camera const&);
    std::ostream& operator<<(std::ostream&, Camera const&);
    std::string to_string(Camera const&);
}

// rendering functions
//
// these perform the necessary backend steps to get something useful done
namespace osc::experimental::Graphics
{
    void DrawMesh(Mesh const&,
                  Transform const&,
                  Material const&,
                  Camera&,
                  std::optional<MaterialPropertyBlock> = std::nullopt);
}

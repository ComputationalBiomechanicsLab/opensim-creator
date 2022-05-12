#pragma once

#include "src/Graphics/Color.hpp"
#include "src/Maths/AABB.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Maths/Transform.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/mat4x3.hpp>
#include <nonstd/span.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <vector>


// 2D texture
//
// representation of an "image" that can be sampled by shaders
namespace osc::experimental
{
    enum class TextureWrapMode {
        Repeat = 0,
        Clamp,
        Mirror,
        TOTAL,
    };

    std::ostream& operator<<(std::ostream&, TextureWrapMode);
    std::string to_string(TextureWrapMode);

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
        friend struct std::hash<Texture2D>;
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

namespace std
{
    template<>
    struct hash<osc::experimental::Texture2D> {
        std::size_t operator()(osc::experimental::Texture2D const&) const;
    };
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
        friend struct std::hash<Shader>;
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

namespace std
{
    template<>
    struct hash<osc::experimental::Shader> {
        std::size_t operator()(osc::experimental::Shader const&) const;
    };
}

// material
//
// pairs a shader (above) with particular properties that the shader will be called with
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
        friend struct std::hash<Material>;
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

namespace std
{
    template<>
    struct hash<osc::experimental::Material> {
        std::size_t operator()(osc::experimental::Material const&) const;
    };
}

// material property block
//
// enables callers to apply per-instance properties to a drawcall (that might be using
// the same material)
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
        friend struct std::hash<MaterialPropertyBlock>;
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

namespace std
{
    template<>
    struct hash<osc::experimental::MaterialPropertyBlock> {
        std::size_t operator()(osc::experimental::MaterialPropertyBlock const&) const;
    };
}

namespace osc::experimental
{
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
        friend struct std::hash<Mesh>;
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

namespace std
{
    template<>
    struct hash<osc::experimental::Mesh> {
        std::size_t operator()(osc::experimental::Mesh const&) const;
    };
}

namespace osc::experimental
{
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
        explicit Camera(Texture2D);  // draws to texture
        Camera(Camera const&);
        Camera(Camera&&) noexcept;
        Camera& operator=(Camera const&);
        Camera& operator=(Camera&&) noexcept;
        ~Camera() noexcept;

        glm::vec4 getBackgroundColor() const;
        void setBackgroundColor(glm::vec4 const&);

        CameraProjection getCameraProjection() const;
        void setCameraProjection(CameraProjection);

        // only used if orthographic
        //
        // e.g. https://docs.unity3d.com/ScriptReference/Camera-orthographicSize.html
        float getOrthographicSize() const;
        void setOrthographicSize(float);

        // only used if perspective
        float getCameraFOV() const;
        void setCameraFOV(float);

        float getNearClippingPlane() const;
        void setNearClippingPlane(float);

        float getFarClippingPlane() const;
        void setFarClippingPlane(float);

        std::optional<Texture2D> getTexture() const;  // returns nullptr if drawing directly to screen
        void setTexture(Texture2D);
        void setTexture();  // resets to drawing to screen

        // where on the screen the camera is rendered (in screen-space)
        //
        // returns rect at 0,0 with width and height of texture if drawing
        // to a texture
        Rect getPixelRect() const;
        void setPixelRect(Rect const&);

        int getPixelWidth() const;
        int getPixelHeight() const;
        float getAspectRatio() const;

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
        friend struct std::hash<Camera>;
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

namespace std
{
    template<>
    struct hash<osc::experimental::Camera> {
        std::size_t operator()(osc::experimental::Camera const&) const;
    };
}

namespace osc::experimental::Graphics
{
    void DrawMesh(Mesh const&,
                  Transform const&,
                  Material const&,
                  Camera&,
                  std::optional<MaterialPropertyBlock> = std::nullopt);
}
#pragma once

#include "src/Graphics/Image.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Utils/CStringView.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>
#include <nonstd/span.hpp>

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct SDL_Window;

namespace osc
{
    struct AABB;
    struct Rgba32;
    struct Transform;
    class Mesh;
    struct MeshData;
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

    // how sampling should handle when the sampling location falls between multiple textels
    enum class TextureFilterMode {
        Nearest = 0,
        Linear,
        TOTAL,
    };
    std::ostream& operator<<(std::ostream&, TextureFilterMode);

    // a handle to a 2D texture that can be rendered by the graphics backend
    class Texture2D final {
    public:
        Texture2D(int width, int height, nonstd::span<Rgba32 const> rgbaPixelsRowByRow);
        Texture2D(int width, int height, nonstd::span<uint8_t const> singleChannelPixelsRowByRow);
        Texture2D(int width, int height, nonstd::span<uint8_t const> channelsRowByRow, int numChannels);
        Texture2D(Texture2D const&);
        Texture2D(Texture2D&&) noexcept;
        Texture2D& operator=(Texture2D const&);
        Texture2D& operator=(Texture2D&&) noexcept;
        ~Texture2D() noexcept;

        int getWidth() const;
        int getHeight() const;
        float getAspectRatio() const;

        TextureWrapMode getWrapMode() const;  // same as getWrapModeU
        void setWrapMode(TextureWrapMode);  // sets all axes
        TextureWrapMode getWrapModeU() const;
        void setWrapModeU(TextureWrapMode);
        TextureWrapMode getWrapModeV() const;
        void setWrapModeV(TextureWrapMode);
        TextureWrapMode getWrapModeW() const;
        void setWrapModeW(TextureWrapMode);

        TextureFilterMode getFilterMode() const;
        void setFilterMode(TextureFilterMode);

    private:
        friend class GraphicsBackend;
        friend bool operator==(Texture2D const&, Texture2D const&);
        friend bool operator!=(Texture2D const&, Texture2D const&);
        friend bool operator<(Texture2D const&, Texture2D const&);
        friend std::ostream& operator<<(std::ostream&, Texture2D const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Texture2D const&, Texture2D const&);
    bool operator!=(Texture2D const&, Texture2D const&);
    bool operator<(Texture2D const&, Texture2D const&);
    std::ostream& operator<<(std::ostream&, Texture2D const&);

    Texture2D LoadTexture2DFromImageResource(std::string_view, ImageFlags = ImageFlags_None);
}


// render texture
//
// a texture that can be rendered to
namespace osc::experimental
{
    enum class RenderTextureFormat {
        ARGB32 = 0,
        RED,
        TOTAL,
    };
    std::ostream& operator<<(std::ostream&, RenderTextureFormat);

    enum class DepthStencilFormat {
        D24_UNorm_S8_UInt = 0,
        TOTAL,
    };
    std::ostream& operator<<(std::ostream&, DepthStencilFormat);

    class RenderTextureDescriptor final {
    public:
        RenderTextureDescriptor(int width, int height);
        RenderTextureDescriptor(RenderTextureDescriptor const&);
        RenderTextureDescriptor(RenderTextureDescriptor&&) noexcept;
        RenderTextureDescriptor& operator=(RenderTextureDescriptor const&);
        RenderTextureDescriptor& operator=(RenderTextureDescriptor&&) noexcept;
        ~RenderTextureDescriptor() noexcept;

        int getWidth() const;
        void setWidth(int);

        int getHeight() const;
        void setHeight(int);

        int getAntialiasingLevel() const;
        void setAntialiasingLevel(int);

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

    private:
        friend class GraphicsBackend;
        friend bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend bool operator!=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend bool operator<(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
        friend std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);

        int m_Width;
        int m_Height;
        int m_AnialiasingLevel;
        RenderTextureFormat m_ColorFormat;
        DepthStencilFormat m_DepthStencilFormat;
    };

    bool operator==(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    bool operator!=(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    bool operator<(RenderTextureDescriptor const&, RenderTextureDescriptor const&);
    std::ostream& operator<<(std::ostream&, RenderTextureDescriptor const&);

    class RenderTexture final {
    public:
        explicit RenderTexture(RenderTextureDescriptor const&);
        RenderTexture(RenderTexture const&);
        RenderTexture(RenderTexture&&) noexcept;
        RenderTexture& operator=(RenderTexture const&);
        RenderTexture& operator=(RenderTexture&&) noexcept;
        ~RenderTexture() noexcept;

        int getWidth() const;
        void setWidth(int);

        int getHeight() const;
        void setHeight(int);

        RenderTextureFormat getColorFormat() const;
        void setColorFormat(RenderTextureFormat);

        int getAntialiasingLevel() const;
        void setAntialiasingLevel(int);

        DepthStencilFormat getDepthStencilFormat() const;
        void setDepthStencilFormat(DepthStencilFormat);

        void reformat(RenderTextureDescriptor const& d);

    private:
        friend class GraphicsBackend;
        friend bool operator==(RenderTexture const&, RenderTexture const&);
        friend bool operator!=(RenderTexture const&, RenderTexture const&);
        friend bool operator<(RenderTexture const&, RenderTexture const&);
        friend std::ostream& operator<<(std::ostream&, RenderTexture const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(RenderTexture const&, RenderTexture const&);
    bool operator!=(RenderTexture const&, RenderTexture const&);
    bool operator<(RenderTexture const&, RenderTexture const&);
    std::ostream& operator<<(std::ostream&, RenderTexture const&);

    void EmplaceOrReformat(std::optional<RenderTexture>& t, RenderTextureDescriptor const& desc);
}


// shader
//
// encapsulates a shader program that has been compiled + initialized by the backend
namespace osc::experimental
{
    // data type of a material-assignable property parsed from the shader code
    enum class ShaderType {
        Float = 0,
        Vec2,
        Vec3,
        Vec4,
        Mat3,
        Mat4,
        Int,
        Bool,
        Sampler2D,
        Unknown,
        TOTAL,
    };
    std::ostream& operator<<(std::ostream&, ShaderType);

    // a handle to a shader
    class Shader final {
    public:
        Shader(CStringView vertexShader, CStringView fragmentShader);  // throws on compile error
        Shader(CStringView vertexShader, CStringView geometryShader, CStringView fragmmentShader);  // throws on compile error
        Shader(Shader const&);
        Shader(Shader&&) noexcept;
        Shader& operator=(Shader const&);
        Shader& operator=(Shader&&) noexcept;
        ~Shader() noexcept;

        std::optional<int> findPropertyIndex(std::string const& propertyName) const;

        int getPropertyCount() const;
        std::string const& getPropertyName(int propertyIndex) const;
        ShaderType getPropertyType(int propertyIndex) const;

    private:
        friend class GraphicsBackend;
        friend bool operator==(Shader const&, Shader const&);
        friend bool operator!=(Shader const&, Shader const&);
        friend bool operator<(Shader const&, Shader const&);
        friend std::ostream& operator<<(std::ostream&, Shader const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Shader const&, Shader const&);
    bool operator!=(Shader const&, Shader const&);
    bool operator<(Shader const&, Shader const&);
    std::ostream& operator<<(std::ostream&, Shader const&);
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

        std::optional<nonstd::span<float const>> getFloatArray(std::string_view propertyName) const;
        void setFloatArray(std::string_view propertyName, nonstd::span<float const>);

        std::optional<glm::vec2> getVec2(std::string_view propertyName) const;
        void setVec2(std::string_view propertyName, glm::vec2);

        std::optional<glm::vec3> getVec3(std::string_view propertyName) const;
        void setVec3(std::string_view propertyName, glm::vec3);

        std::optional<nonstd::span<glm::vec3 const>> getVec3Array(std::string_view propertyName) const;
        void setVec3Array(std::string_view propertyName, nonstd::span<glm::vec3 const>);

        std::optional<glm::vec4> getVec4(std::string_view propertyName) const;
        void setVec4(std::string_view propertyName, glm::vec4);

        std::optional<glm::mat3> getMat3(std::string_view propertyName) const;
        void setMat3(std::string_view propertyName, glm::mat3 const&);

        std::optional<glm::mat4> getMat4(std::string_view propertyName) const;
        void setMat4(std::string_view propertyName, glm::mat4 const&);

        std::optional<int> getInt(std::string_view propertyName) const;
        void setInt(std::string_view propertyName, int);

        std::optional<bool> getBool(std::string_view propertyName) const;
        void setBool(std::string_view propertyName, bool);

        std::optional<Texture2D> getTexture(std::string_view propertyName) const;
        void setTexture(std::string_view propertyName, Texture2D);

        std::optional<RenderTexture> getRenderTexture(std::string_view propertyName) const;
        void setRenderTexture(std::string_view propertyName, RenderTexture);
        void clearRenderTexture(std::string_view propertyName);

        bool getTransparent() const;
        void setTransparent(bool);

        bool getDepthTested() const;
        void setDepthTested(bool);

    private:
        friend class GraphicsBackend;
        friend bool operator==(Material const&, Material const&);
        friend bool operator!=(Material const&, Material const&);
        friend bool operator<(Material const&, Material const&);
        friend std::ostream& operator<<(std::ostream&, Material const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Material const&, Material const&);
    bool operator!=(Material const&, Material const&);
    bool operator<(Material const&, Material const&);
    std::ostream& operator<<(std::ostream&, Material const&);
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

        std::optional<int> getInt(std::string_view propertyName) const;
        void setInt(std::string_view, int);

        std::optional<bool> getBool(std::string_view propertyName) const;
        void setBool(std::string_view propertyName, bool);

        std::optional<Texture2D> getTexture(std::string_view propertyName) const;
        void setTexture(std::string_view, Texture2D);

    private:
        friend class GraphicsBackend;
        friend bool operator==(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend bool operator!=(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend bool operator<(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
        friend std::ostream& operator<<(std::ostream&, MaterialPropertyBlock const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    bool operator!=(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    bool operator<(MaterialPropertyBlock const&, MaterialPropertyBlock const&);
    std::ostream& operator<<(std::ostream&, MaterialPropertyBlock const&);
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

        nonstd::span<Rgba32 const> getColors();
        void setColors(nonstd::span<Rgba32 const>);

        int getNumIndices() const;
        std::vector<std::uint32_t> getIndices() const;
        void setIndices(nonstd::span<std::uint16_t const>);
        void setIndices(nonstd::span<std::uint32_t const>);

        AABB const& getBounds() const;  // local-space

        glm::vec3 getMidpoint() const;  // local-space

        void clear();

    private:
        friend class GraphicsBackend;
        friend bool operator==(Mesh const&, Mesh const&);
        friend bool operator!=(Mesh const&, Mesh const&);
        friend bool operator<(Mesh const&, Mesh const&);
        friend std::ostream& operator<<(std::ostream&, Mesh const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Mesh const&, Mesh const&);
    bool operator!=(Mesh const&, Mesh const&);
    bool operator<(Mesh const&, Mesh const&);
    std::ostream& operator<<(std::ostream&, Mesh const&);

    Mesh LoadMeshFromMeshData(MeshData const&);
    osc::experimental::Mesh LoadMeshFromLegacyMesh(osc::Mesh const&);
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

    enum class CameraClearFlags {
        SolidColor = 0,
        Depth,
        Nothing,
        TOTAL,
        Default = SolidColor,
    };

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

        CameraClearFlags getClearFlags() const;
        void setClearFlags(CameraClearFlags);

        std::optional<RenderTexture> getTexture() const; // empty if drawing directly to screen
        void setTexture(RenderTexture&&);
        void setTexture(RenderTextureDescriptor);
        void setTexture();  // resets to drawing to screen
        void swapTexture(std::optional<RenderTexture>&);  // handy if the caller wants to handle the textures

        // where on the screen the camera is rendered (in screen-space - top-left, X rightwards, Y down
        //
        // returns rect with topleft coord 0,0  and a width/height of texture if drawing
        // to a texture
        Rect getPixelRect() const;
        void setPixelRect(Rect const&);
        void setPixelRect();

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

        // get rotation (from the assumed "default" rotation of the camera pointing towards -Z, Y is up)
        glm::quat getRotation() const;
        void setRotation(glm::quat const&);

        glm::vec3 getDirection() const;
        glm::vec3 getUpwardsDirection() const;

        // view matrix (overrides)
        //
        // the caller can manually override the view matrix, which can be handy in certain
        // rendering scenarios. Use `resetViewMatrix` to return to using position, direction,
        // and upwards direction
        glm::mat4 getViewMatrix() const;
        void setViewMatrix(glm::mat4 const&);
        void resetViewMatrix();

        // projection matrix (overrides)
        //
        // the caller can manually override the projection matrix, which can be handy in certain
        // rendering scenarios. Use `resetProjectionMatrix` to return to using position, direction,
        // and upwards direction
        glm::mat4 getProjectionMatrix() const;
        void setProjectionMatrix(glm::mat4 const&);
        void resetProjectionMatrix();

        // returns getProjectionMatrix() * getViewMatrix()
        glm::mat4 getViewProjectionMatrix() const;

        // flushes any rendering commands that were queued against this camera
        //
        // after this call completes, the output texture, or screen, should contain
        // the rendered geometry
        void render();

    private:
        friend class GraphicsBackend;
        friend bool operator==(Camera const&, Camera const&);
        friend bool operator!=(Camera const&, Camera const&);
        friend bool operator<(Camera const&, Camera const&);
        friend std::ostream& operator<<(std::ostream&, Camera const&);

        class Impl;
        std::shared_ptr<Impl> m_Impl;
    };

    bool operator==(Camera const&, Camera const&);
    bool operator!=(Camera const&, Camera const&);
    bool operator<(Camera const&, Camera const&);
    std::ostream& operator<<(std::ostream&, Camera const&);
}

// graphics context
//
// should be initialized exactly once by the application
namespace osc::experimental
{
    class GraphicsContext final {
    public:
        GraphicsContext(SDL_Window*);
        GraphicsContext(GraphicsContext const&) = delete;
        GraphicsContext(GraphicsContext&&) noexcept = delete;
        GraphicsContext& operator=(GraphicsContext const&) = delete;
        GraphicsContext& operator=(GraphicsContext&&) noexcept = delete;
        ~GraphicsContext() noexcept;

        int getMaxMSXAASamples() const;

        bool isVsyncEnabled() const;
        void enableVsync();
        void disableVsync();

        bool isInDebugMode() const;
        void enableDebugMode();
        void disableDebugMode();

        void clearProgram();
        void clearScreen(glm::vec4 const&);

        void* updRawGLContextHandle();  // needed by ImGui

        class Impl;
    private:
        // no data - it uses globals (you can only have one of these)
    };
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
    void DrawMesh(Mesh const&,
        glm::mat4 const&,
        Material const&,
        Camera&,
        std::optional<MaterialPropertyBlock> = std::nullopt);
}

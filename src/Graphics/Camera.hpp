#pragma once

#include "src/Graphics/CameraClearFlags.hpp"
#include "src/Graphics/CameraProjection.hpp"
#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/RenderTextureDescriptor.hpp"
#include "src/Maths/Rect.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <optional>

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // camera
    //
    // encapsulates a camera viewport that can be drawn to, with the intention of producing
    // a 2D rendered image of the drawn elements
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
        int32_t getPixelWidth() const;
        int32_t getPixelHeight() const;
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

        // careful: the camera doesn't *store* a direction vector - it assumes the direction is along -Z,
        // and that +Y is "upwards" and figures out how to rotate from that to your desired direction
        //
        // if you want to "roll" the camera (i.e. Y isn't upwards) then use `setRotation`
        glm::vec3 getDirection() const;
        void setDirection(glm::vec3 const&);

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

        // returns the equivalent of getProjectionMatrix() * getViewMatrix()
        glm::mat4 getViewProjectionMatrix() const;

        // returns the equivalent of inverse(getViewProjectionMatrix())
        glm::mat4 getInverseViewProjectionMatrix() const;

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
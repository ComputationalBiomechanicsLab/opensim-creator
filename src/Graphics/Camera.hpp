#pragma once

#include "src/Graphics/CameraClearFlags.hpp"
#include "src/Graphics/CameraProjection.hpp"
#include "src/Maths/Rect.hpp"
#include "src/Utils/CopyOnUpdPtr.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <iosfwd>
#include <optional>

namespace osc { class RenderTexture; }

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // camera
    //
    // encapsulates a camera viewport that can be drawn to, with the intention of producing
    // a 2D rendered image of the drawn elements
    class Camera final {
    public:
        Camera();
        Camera(Camera const&);
        Camera(Camera&&) noexcept;
        Camera& operator=(Camera const&);
        Camera& operator=(Camera&&) noexcept;
        ~Camera() noexcept;

        // reset to default parameters
        void reset();

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

        // where on the screen/texture that the camera should render the viewport to
        //
        // the rect uses a top-left coordinate system (in screen-space - top-left, X rightwards, Y down)
        //
        // if this is not specified, the camera will render to the full extents of the given
        // render output (entire screen, or entire render texture)
        std::optional<Rect> getPixelRect() const;
        void setPixelRect(std::optional<Rect>);

        // scissor testing
        //
        // This tells the rendering backend to only render the fragments that occur within
        // these bounds. It's useful when (e.g.) running an expensive fragment shader (e.g.
        // image processing kernels) where you know that only a certain subspace is actually
        // interesting (e.g. rim-highlighting only selected elements)
        std::optional<Rect> getScissorRect() const;  // std::nullopt if not scissor testing
        void setScissorRect(std::optional<Rect>);

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

        // get view matrix
        //
        // the caller can manually override the view matrix, which can be handy in certain
        // rendering scenarios
        glm::mat4 getViewMatrix() const;
        std::optional<glm::mat4> getViewMatrixOverride() const;
        void setViewMatrixOverride(std::optional<glm::mat4>);

        // projection matrix
        //
        // the caller can manually override the projection matrix, which can be handy in certain
        // rendering scenarios.
        glm::mat4 getProjectionMatrix(float aspectRatio) const;
        std::optional<glm::mat4> getProjectionMatrixOverride() const;
        void setProjectionMatrixOverride(std::optional<glm::mat4>);

        // returns the equivalent of getProjectionMatrix(aspectRatio) * getViewMatrix()
        glm::mat4 getViewProjectionMatrix(float aspectRatio) const;

        // returns the equivalent of inverse(getViewProjectionMatrix(aspectRatio))
        glm::mat4 getInverseViewProjectionMatrix(float aspectRatio) const;

        // flushes any rendering commands that were queued against this camera
        //
        // after this call completes, the output texture, or screen, should contain
        // the rendered geometry
        void renderToScreen();
        void renderTo(RenderTexture&);

        friend void swap(Camera& a, Camera& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

    private:
        friend class GraphicsBackend;
        friend bool operator==(Camera const&, Camera const&) noexcept;
        friend bool operator!=(Camera const&, Camera const&) noexcept;
        friend std::ostream& operator<<(std::ostream&, Camera const&);

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    bool operator==(Camera const&, Camera const&) noexcept;
    bool operator!=(Camera const&, Camera const&) noexcept;
    std::ostream& operator<<(std::ostream&, Camera const&);
}
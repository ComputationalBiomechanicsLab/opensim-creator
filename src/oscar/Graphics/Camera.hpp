#pragma once

#include <oscar/Graphics/CameraClearFlags.hpp>
#include <oscar/Graphics/CameraProjection.hpp>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/Angle.hpp>
#include <oscar/Maths/Mat4.hpp>
#include <oscar/Maths/Quat.hpp>
#include <oscar/Maths/Rect.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Utils/CopyOnUpdPtr.hpp>

#include <iosfwd>
#include <optional>

namespace osc { class RenderTexture; }
namespace osc { struct RenderTarget; }

// note: implementation is in `GraphicsImplementation.cpp`
namespace osc
{
    // camera
    //
    // encapsulates a camera viewport that can be drawn to, with the intention of producing
    // a 2D rendered image of the drawn elements
    class Camera {
    public:
        Camera();

        // reset to default parameters
        void reset();

        Color getBackgroundColor() const;
        void setBackgroundColor(Color const&);

        CameraProjection getCameraProjection() const;
        void setCameraProjection(CameraProjection);

        // only used if CameraProjection == Orthographic
        float getOrthographicSize() const;
        void setOrthographicSize(float);

        // only used if CameraProjection == Perspective
        Radians getVerticalFOV() const;
        void setVerticalFOV(Radians);

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

        Vec3 getPosition() const;
        void setPosition(Vec3 const&);

        // get rotation (from the assumed "default" rotation of the camera pointing towards -Z, Y is up)
        Quat getRotation() const;
        void setRotation(Quat const&);

        // careful: the camera doesn't *store* a direction vector - it assumes the direction is along -Z,
        // and that +Y is "upwards" and figures out how to rotate from that to your desired direction
        //
        // if you want to "roll" the camera (i.e. Y isn't upwards) then use `setRotation`
        Vec3 getDirection() const;
        void setDirection(Vec3 const&);

        Vec3 getUpwardsDirection() const;

        // get view matrix
        //
        // the caller can manually override the view matrix, which can be handy in certain
        // rendering scenarios
        Mat4 getViewMatrix() const;
        std::optional<Mat4> getViewMatrixOverride() const;
        void setViewMatrixOverride(std::optional<Mat4>);

        // projection matrix
        //
        // the caller can manually override the projection matrix, which can be handy in certain
        // rendering scenarios.
        Mat4 getProjectionMatrix(float aspectRatio) const;
        std::optional<Mat4> getProjectionMatrixOverride() const;
        void setProjectionMatrixOverride(std::optional<Mat4>);

        // returns the equivalent of getProjectionMatrix(aspectRatio) * getViewMatrix()
        Mat4 getViewProjectionMatrix(float aspectRatio) const;

        // returns the equivalent of inverse(getViewProjectionMatrix(aspectRatio))
        Mat4 getInverseViewProjectionMatrix(float aspectRatio) const;

        // flushes any rendering commands that were queued against this camera
        //
        // after this call completes, the output texture, or screen, should contain
        // the rendered geometry
        void renderToScreen();
        void renderTo(RenderTexture&);
        void renderTo(RenderTarget&);

        friend void swap(Camera& a, Camera& b) noexcept
        {
            swap(a.m_Impl, b.m_Impl);
        }

    private:
        friend bool operator==(Camera const&, Camera const&);
        friend std::ostream& operator<<(std::ostream&, Camera const&);
        friend class GraphicsBackend;

        class Impl;
        CopyOnUpdPtr<Impl> m_Impl;
    };

    bool operator==(Camera const&, Camera const&);
    std::ostream& operator<<(std::ostream&, Camera const&);
}

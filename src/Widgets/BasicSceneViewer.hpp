#pragma once

#include "src/Graphics/BasicSceneElement.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

namespace osc
{
    class PolarPerspectiveCamera;
}

namespace osc
{
    using BasicSceneViewerFlags = int;
    enum BasicSceneViewerFlags_ {
        BasicSceneViewerFlags_None = 0,

        // update the camera when the user clicks+drags etc. over the scene image
        BasicSceneViewerFlags_UpdateCameraFromImGuiInput = 1<<0,

        BasicSceneViewerFlags_Default = BasicSceneViewerFlags_UpdateCameraFromImGuiInput,
    };

    class BasicSceneViewer final {
    public:
        explicit BasicSceneViewer(BasicSceneViewerFlags = BasicSceneViewerFlags_Default);
        BasicSceneViewer(BasicSceneViewer const&) = delete;
        BasicSceneViewer(BasicSceneViewer&&) noexcept;
        BasicSceneViewer& operator=(BasicSceneViewer const&) = delete;
        BasicSceneViewer& operator=(BasicSceneViewer&&) noexcept;
        ~BasicSceneViewer() noexcept;

        void setDimensions(glm::ivec2 dimensions);
        void setSamples(int samples);
        PolarPerspectiveCamera& updCamera();

        void draw(nonstd::span<BasicSceneElement const>);

        bool isHovered() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;

        class Impl;
    private:
        Impl* m_Impl;
    };
}
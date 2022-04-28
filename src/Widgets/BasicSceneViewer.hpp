#pragma once

#include "src/Graphics/BasicRenderer.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <memory>

namespace osc
{
    struct BasicSceneElement;
}

namespace osc
{
    // pumps scenes into a `osc::BasicRenderer` and emits the output as an `ImGui::Image()`
    class BasicSceneViewer final {
    public:
        BasicSceneViewer();
        explicit BasicSceneViewer(std::unique_ptr<BasicRenderer>);
        BasicSceneViewer(BasicSceneViewer const&) = delete;
        BasicSceneViewer(BasicSceneViewer&&) noexcept;
        BasicSceneViewer& operator=(BasicSceneViewer const&) = delete;
        BasicSceneViewer& operator=(BasicSceneViewer&&) noexcept;
        ~BasicSceneViewer() noexcept;

        glm::ivec2 getDimensions() const;
        void setDimensions(glm::ivec2);
        int getSamples() const;
        void setSamples(int samples);

        void draw(BasicRenderer::Params const&, nonstd::span<BasicSceneElement const>);

        bool isHovered() const;
        bool isLeftClicked() const;
        bool isRightClicked() const;

        class Impl;
    private:
        Impl* m_Impl;
    };
}
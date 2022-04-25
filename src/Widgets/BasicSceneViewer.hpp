#pragma once

#include "src/Graphics/BasicSceneElement.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

namespace osc
{
    class BasicSceneViewer final {
    public:
        explicit BasicSceneViewer();
        BasicSceneViewer(BasicSceneViewer const&) = delete;
        BasicSceneViewer(BasicSceneViewer&&) noexcept;
        BasicSceneViewer& operator=(BasicSceneViewer const&) = delete;
        BasicSceneViewer& operator=(BasicSceneViewer&&) noexcept;
        ~BasicSceneViewer() noexcept;

        void setDimensions(glm::ivec2 dimensions);
        void setSamples(int samples);
        void draw(nonstd::span<BasicSceneElement const>);
        bool isHovered() const;

        class Impl;
    private:
        Impl* m_Impl;
    };
}
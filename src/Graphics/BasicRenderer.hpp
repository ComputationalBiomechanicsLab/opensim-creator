#pragma once

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

namespace gl
{
	class Texture2D;
}

namespace osc
{
    class BasicRendererParams;
    class BasicSceneElement;
}

namespace osc
{
	class BasicRenderer {
	public:
        BasicRenderer() = default;
        BasicRenderer(BasicRenderer const&) = delete;
        BasicRenderer(BasicRenderer&&) noexcept = delete;
        BasicRenderer& operator=(BasicRenderer const&) = delete;
        BasicRenderer& operator=(BasicRenderer&&) noexcept = delete;
		virtual ~BasicRenderer() noexcept = default;

		virtual glm::ivec2 getDimensions() const = 0;
		virtual void setDimensions(glm::ivec2) = 0;
		virtual int getSamples() const = 0;
		virtual void setSamples(int) = 0;
        virtual void draw(BasicRendererParams const&, nonstd::span<BasicSceneElement const>) = 0;
		virtual gl::Texture2D& updOutputTexture() = 0;
	};
}

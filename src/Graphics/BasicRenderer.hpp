#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <nonstd/span.hpp>

namespace gl
{
	class Texture2D;
}

namespace osc
{
	struct BasicSceneElement;
}

namespace osc
{
	class BasicRenderer {
	public:
		struct Params {
			glm::vec3 lightDirection{1.0f, 0.0f, 0.0f};
			glm::mat4 projectionMatrix{1.0f};
			glm::mat4 viewMatrix{1.0f};
			glm::vec3 viewPos{0.0f, 0.0f, 0.0f};
			glm::vec3 lightColor{248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f};
		};

		virtual ~BasicRenderer() noexcept = default;
		virtual glm::ivec2 getDimensions() const = 0;
		virtual void setDimensions(glm::ivec2) = 0;
		virtual int getSamples() const = 0;
		virtual void setSamples(int) = 0;
		virtual void draw(Params const&, nonstd::span<BasicSceneElement const>) = 0;
		virtual gl::Texture2D& updOutputTexture() = 0;
	};
}
#pragma once

#include "src/Graphics/Gl.hpp"

#include <glm/vec2.hpp>

namespace osc
{
	class MultisampledRenderBuffers final {
	public:
		MultisampledRenderBuffers(glm::ivec2 dims, int samples);

		void setDimsAndSamples(glm::ivec2 newDims, int newSamples);

		int getWidth() const
		{
			return m_Dimensions.x;
		}

		int getHeight() const
		{
			return m_Dimensions.y;
		}

		glm::vec2 getDimensionsf() const
		{
			return {static_cast<float>(m_Dimensions.x), static_cast<float>(m_Dimensions.y)};
		}

		float getAspectRatio() const
		{
			glm::vec2 dims = getDimensionsf();
			return dims.x / dims.y;
		}

		gl::FrameBuffer& updRenderingFBO()
		{
			return m_FrameBuffer;
		}

		gl::FrameBuffer& updSceneFBO()
		{
			return m_SceneFrameBuffer;
		}

		gl::Texture2D& updSceneTexture()
		{
			return m_SceneTexture;
		}

	private:
		glm::ivec2 m_Dimensions;
		int m_Samples;
		gl::RenderBuffer m_SceneRBO;
		gl::RenderBuffer m_Depth24StencilRBO;
		gl::FrameBuffer m_FrameBuffer;
		gl::Texture2D m_SceneTexture;
		gl::FrameBuffer m_SceneFrameBuffer;
	};
}
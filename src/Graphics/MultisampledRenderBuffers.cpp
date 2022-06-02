#include "MultisampledRenderBuffers.hpp"

#include "src/Graphics/Gl.hpp"

#include <glm/vec2.hpp>

#include <utility>

osc::MultisampledRenderBuffers::MultisampledRenderBuffers(glm::ivec2 dims_, int samples_) :
	m_Dimensions{std::move(dims_)},
	m_Samples{std::move(samples_)},
	m_SceneRBO{[this]()
	{
		gl::RenderBuffer rv;
		gl::BindRenderBuffer(rv);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_Samples, GL_RGBA, m_Dimensions.x, m_Dimensions.y);
		return rv;
	}()},
	m_Depth24StencilRBO{[this]()
	{
		gl::RenderBuffer rv;
		gl::BindRenderBuffer(rv);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_Samples, GL_DEPTH24_STENCIL8, m_Dimensions.x, m_Dimensions.y);
		return rv;
	}()},
	m_FrameBuffer{[this]()
	{
		gl::FrameBuffer rv;
		gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
		gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_SceneRBO);
		gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, m_Depth24StencilRBO);
		gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
		return rv;
	}()},
	m_SceneTexture{[this]()
	{
		gl::Texture2D rv;
		gl::BindTexture(rv);
		gl::TexImage2D(rv.type, 0, GL_RGBA, m_Dimensions.x, m_Dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		gl::TexParameteri(rv.type, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		gl::TexParameteri(rv.type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		gl::TexParameteri(rv.type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		return rv;
	}()},
	m_SceneFrameBuffer{[this]()
	{
		gl::FrameBuffer rv;
		gl::BindFramebuffer(GL_FRAMEBUFFER, rv);
		gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_SceneTexture, 0);
		gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
		return rv;
	}()}
{
}

void osc::MultisampledRenderBuffers::setDimsAndSamples(glm::ivec2 newDims, int newSamples)
{
	if (newDims != m_Dimensions || newSamples != m_Samples)
	{
		*this = MultisampledRenderBuffers{newDims, newSamples};
	}
}
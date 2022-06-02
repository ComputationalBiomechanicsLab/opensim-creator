#include "SceneRenderer.hpp"

#include "src/Graphics/BasicSceneElement.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/MultisampledRenderBuffers.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Graphics/Shaders/GouraudShader.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/Transform.hpp"
#include "src/Platform/App.hpp"

#include <glm/vec2.hpp>
#include <nonstd/span.hpp>

#include <optional>
#include <utility>

class osc::SceneRenderer::Impl final {
public:
	Impl() = default;

	glm::ivec2 getDimensions() const
	{
		return m_RequestedDimensions;
	}

	void setDimensions(glm::ivec2 newDimensions)
	{
		m_RequestedDimensions = std::move(newDimensions);
	}

	int getSamples()
	{
		return m_RequestedSamples;
	}

	void setSamples(int newSamples)
	{
		m_RequestedSamples = std::move(newSamples);
	}

	void draw(BasicRenderer::Params const& params, nonstd::span<BasicSceneElement const> els)
	{
		if (!m_MaybeRenderBuffers)
		{
			m_MaybeRenderBuffers.emplace(m_RequestedDimensions, m_RequestedSamples);
		}
		else
		{
			m_MaybeRenderBuffers->setDimsAndSamples(m_RequestedDimensions, m_RequestedSamples);
		}

		MultisampledRenderBuffers& bufs = *m_MaybeRenderBuffers;

		// setup any rendering state vars
		gl::Viewport(0, 0, bufs.getWidth(), bufs.getHeight());
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		gl::Enable(GL_BLEND);
		gl::Enable(GL_DEPTH_TEST);
		gl::BindFramebuffer(GL_FRAMEBUFFER, bufs.updRenderingFBO());
		gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// bind to shader and setup uniforms
		gl::UseProgram(m_Shader.program);
		gl::Uniform(m_Shader.uProjMat, params.projectionMatrix);
		gl::Uniform(m_Shader.uViewMat, params.viewMatrix);
		gl::Uniform(m_Shader.uLightDir, params.lightDirection);
		gl::Uniform(m_Shader.uLightColor, params.lightColor);
		gl::Uniform(m_Shader.uViewPos, params.viewPos);
		gl::Uniform(m_Shader.uIsTextured, false);

		for (BasicSceneElement const& el : els)
		{
			gl::Uniform(m_Shader.uModelMat, ToMat4(el.transform));
			gl::Uniform(m_Shader.uNormalMat, ToNormalMatrix(el.transform));
			gl::Uniform(m_Shader.uDiffuseColor, el.color);
			gl::BindVertexArray(el.mesh->GetVertexArray());
			el.mesh->Draw();
		}
		gl::BindVertexArray();

		// blit MSXAAed RenderBuffer to non-MSXAAed Texture
		gl::BindFramebuffer(GL_READ_FRAMEBUFFER, bufs.updRenderingFBO());
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		gl::BindFramebuffer(GL_DRAW_FRAMEBUFFER, bufs.updSceneFBO());
		gl::DrawBuffer(GL_COLOR_ATTACHMENT0);
		gl::BlitFramebuffer(0, 0, bufs.getWidth(), bufs.getHeight(), 0, 0, bufs.getWidth(), bufs.getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);

		gl::BindFramebuffer(GL_FRAMEBUFFER, gl::windowFbo);
	}

	gl::Texture2D& updOutputTexture()
	{
		return m_MaybeRenderBuffers->updSceneTexture();
	}

private:
	glm::ivec2 m_RequestedDimensions = {0, 0};
	int m_RequestedSamples = 1;
	GouraudShader& m_Shader = osc::App::shader<GouraudShader>();
	std::optional<MultisampledRenderBuffers> m_MaybeRenderBuffers;
};

osc::SceneRenderer::SceneRenderer() :
	m_Impl{new Impl{}}
{
}

osc::SceneRenderer::SceneRenderer(SceneRenderer&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::SceneRenderer& osc::SceneRenderer::operator=(SceneRenderer&& tmp) noexcept
{
	std::swap(m_Impl, tmp.m_Impl);
	return *this;
}

osc::SceneRenderer::~SceneRenderer() noexcept
{
	delete m_Impl;
}

glm::ivec2 osc::SceneRenderer::getDimensions() const
{
	return m_Impl->getDimensions();
}

void osc::SceneRenderer::setDimensions(glm::ivec2 newDims)
{
	m_Impl->setDimensions(std::move(newDims));
}

int osc::SceneRenderer::getSamples() const
{
	return m_Impl->getSamples();
}

void osc::SceneRenderer::setSamples(int newSamples)
{
	m_Impl->setSamples(std::move(newSamples));
}

void osc::SceneRenderer::draw(BasicRenderer::Params const& params, nonstd::span<BasicSceneElement const> els)
{
	m_Impl->draw(params, els);
}

gl::Texture2D& osc::SceneRenderer::updOutputTexture()
{
	return m_Impl->updOutputTexture();
}
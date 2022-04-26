#include "BasicSceneViewer.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/Gl.hpp"
#include "src/Graphics/GlGlm.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/Shaders/GouraudShader.hpp"
#include "src/Maths/Constants.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Maths/PolarPerspectiveCamera.hpp"
#include "src/Platform/App.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <nonstd/span.hpp>

#include <optional>
#include <utility>

namespace
{
	class RenderBuffers final {
	public:
		RenderBuffers(glm::ivec2 dims_, int samples_) :
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

		void setDimsAndSamples(glm::ivec2 newDims, int newSamples)
		{
			if (newDims != m_Dimensions || newSamples != m_Samples)
			{
				*this = RenderBuffers{newDims, newSamples};
			}
		}

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

class osc::BasicSceneViewer::Impl final {
public:
	explicit Impl(BasicSceneViewerFlags flags) :
		m_Flags{std::move(flags)}
	{
		m_Camera.radius = 5.0f;
	}

	void setDimensions(glm::ivec2 dimensions)
	{
		m_RequestedDimensions = std::move(dimensions);
	}

	void setSamples(int samples)
	{
		m_RequestedSamples = std::move(samples);
	}

	PolarPerspectiveCamera& updCamera()
	{
		return m_Camera;
	}

	void draw(nonstd::span<BasicSceneElement const> els)
	{
		if (m_RequestedDimensions.x <= 0 || m_RequestedDimensions.y <= 0)
		{
			ImGui::Text("error: dimensions of viewer are invalid: have you called BasicSceneViewer::setDimensions yet?");
			return;
		}

		if (m_RequestedSamples <= 0)
		{
			ImGui::Text("error: invalid number of samples requested: must call BasicSceneViewer::setSamples with a positive number (i.e. 1, 2, 4, 8, 16)");
			return;
		}

		if (m_Flags & BasicSceneViewerFlags_UpdateCameraFromImGuiInput && m_IsHovered)
		{
			osc::UpdatePolarCameraFromImGuiUserInput(m_RequestedDimensions, m_Camera);
		}

		if (!m_MaybeRenderBuffers)
		{
			m_MaybeRenderBuffers.emplace(m_RequestedDimensions, m_RequestedSamples);
		}
		else
		{
			m_MaybeRenderBuffers->setDimsAndSamples(m_RequestedDimensions, m_RequestedSamples);
		}

		// automatically change lighting position based on camera position
		glm::vec3 lightDir;
		{
			glm::vec3 p = glm::normalize(-m_Camera.focusPoint - m_Camera.getPos());
			glm::vec3 up = {0.0f, 1.0f, 0.0f};
			glm::vec3 mp = glm::rotate(glm::mat4{1.0f}, 1.25f * fpi4, up) * glm::vec4{p, 0.0f};
			lightDir = glm::normalize(mp + -up);
		}

		RenderBuffers& bufs = *m_MaybeRenderBuffers;

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
		gl::Uniform(m_Shader.uProjMat, m_Camera.getProjMtx(bufs.getAspectRatio()));
		gl::Uniform(m_Shader.uViewMat, m_Camera.getViewMtx());
		gl::Uniform(m_Shader.uLightDir, lightDir);
		gl::Uniform(m_Shader.uLightColor, { 248.0f / 255.0f, 247.0f / 255.0f, 247.0f / 255.0f });
		gl::Uniform(m_Shader.uViewPos, m_Camera.getPos());
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

		// emit the texture to ImGui
		osc::DrawTextureAsImGuiImage(bufs.updSceneTexture(), bufs.getDimensionsf());
		m_IsHovered = ImGui::IsItemHovered();
		m_IsLeftClicked = ImGui::IsItemHovered() && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Left);
		m_IsRightClicked = ImGui::IsItemHovered() && osc::IsMouseReleasedWithoutDragging(ImGuiMouseButton_Right);
	}

	bool isHovered() const
	{
		return m_IsHovered;
	}

	bool isLeftClicked() const
	{
		return m_IsLeftClicked;
	}

	bool isRightClicked() const
	{
		return m_IsRightClicked;
	}

private:
	BasicSceneViewerFlags m_Flags = BasicSceneViewerFlags_Default;
	glm::ivec2 m_RequestedDimensions = {0, 0};
	int m_RequestedSamples = 1;
	PolarPerspectiveCamera m_Camera;
	GouraudShader& m_Shader = osc::App::shader<GouraudShader>();
	std::optional<RenderBuffers> m_MaybeRenderBuffers;
	bool m_IsHovered = false;
	bool m_IsLeftClicked = false;
	bool m_IsRightClicked = false;
};

osc::BasicSceneViewer::BasicSceneViewer(BasicSceneViewerFlags flags) :
	m_Impl{new Impl{std::move(flags)}}
{
}

osc::BasicSceneViewer::BasicSceneViewer(BasicSceneViewer&& tmp) noexcept :
	m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}
osc::BasicSceneViewer& osc::BasicSceneViewer::operator=(BasicSceneViewer&& tmp) noexcept
{
	std::swap(tmp.m_Impl, m_Impl);
	return *this;
}

osc::BasicSceneViewer::~BasicSceneViewer() noexcept
{
	delete m_Impl;
}

void osc::BasicSceneViewer::setDimensions(glm::ivec2 dimensions)
{
	m_Impl->setDimensions(std::move(dimensions));
}

void osc::BasicSceneViewer::setSamples(int samples)
{
	m_Impl->setSamples(std::move(samples));
}

osc::PolarPerspectiveCamera& osc::BasicSceneViewer::updCamera()
{
	return m_Impl->updCamera();
}

void osc::BasicSceneViewer::draw(nonstd::span<BasicSceneElement const> els)
{
	m_Impl->draw(std::move(els));
}

bool osc::BasicSceneViewer::isHovered() const
{
	return m_Impl->isHovered();
}

bool osc::BasicSceneViewer::isLeftClicked() const
{
	return m_Impl->isLeftClicked();
}

bool osc::BasicSceneViewer::isRightClicked() const
{
	return m_Impl->isRightClicked();
}
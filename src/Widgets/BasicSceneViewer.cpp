#include "BasicSceneViewer.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/BasicRenderer.hpp"
#include "src/Graphics/BasicSceneElement.hpp"
#include "src/Graphics/SceneRenderer.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <nonstd/span.hpp>

#include <memory>
#include <utility>

class osc::BasicSceneViewer::Impl final {
public:
	Impl() = default;

	explicit Impl(std::unique_ptr<BasicRenderer> renderer) : m_Renderer{std::move(renderer)}
	{
	}

	glm::ivec2 getDimensions() const
	{
		return m_Renderer->getDimensions();
	}

	void setDimensions(glm::ivec2 dimensions)
	{
		m_Renderer->setDimensions(std::move(dimensions));
	}

	int getSamples() const
	{
		return m_Renderer->getSamples();
	}

	void setSamples(int samples)
	{
		m_Renderer->setSamples(std::move(samples));
	}

	void draw(BasicRenderer::Params const& params, nonstd::span<BasicSceneElement const> els)
	{
		if (m_Renderer->getDimensions().x <= 0 || m_Renderer->getDimensions().y <= 0)
		{
			ImGui::Text("error: dimensions of viewer are invalid: have you called BasicSceneViewer::setDimensions yet?");
			return;
		}

		if (m_Renderer->getSamples() <= 0)
		{
			ImGui::Text("error: invalid number of samples requested: must call BasicSceneViewer::setSamples with a positive number (i.e. 1, 2, 4, 8, 16)");
			return;
		}

		m_Renderer->draw(params, els);

		// emit the texture to ImGui
		osc::DrawTextureAsImGuiImage(m_Renderer->updOutputTexture(), m_Renderer->getDimensions());
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
	std::unique_ptr<BasicRenderer> m_Renderer = std::make_unique<SceneRenderer>();
	bool m_IsHovered = false;
	bool m_IsLeftClicked = false;
	bool m_IsRightClicked = false;
};

osc::BasicSceneViewer::BasicSceneViewer() :
	m_Impl{new Impl{}}
{
}

osc::BasicSceneViewer::BasicSceneViewer(std::unique_ptr<BasicRenderer> renderer) :
	m_Impl{new Impl{std::move(renderer)}}
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

glm::ivec2 osc::BasicSceneViewer::getDimensions() const
{
	return m_Impl->getDimensions();
}

void osc::BasicSceneViewer::setDimensions(glm::ivec2 dimensions)
{
	m_Impl->setDimensions(std::move(dimensions));
}

int osc::BasicSceneViewer::getSamples() const
{
	return m_Impl->getSamples();
}

void osc::BasicSceneViewer::setSamples(int samples)
{
	m_Impl->setSamples(std::move(samples));
}

void osc::BasicSceneViewer::draw(BasicRenderer::Params const& params, nonstd::span<BasicSceneElement const> els)
{
	m_Impl->draw(params, std::move(els));
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
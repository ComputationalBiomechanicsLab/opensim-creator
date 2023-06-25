#include "SceneViewer.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/MeshCache.hpp"
#include "oscar/Graphics/SceneDecoration.hpp"
#include "oscar/Graphics/SceneRenderer.hpp"
#include "oscar/Graphics/SceneRendererParams.hpp"
#include "oscar/Graphics/ShaderCache.hpp"
#include "oscar/Platform/App.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <nonstd/span.hpp>

#include <memory>
#include <utility>

class osc::SceneViewer::Impl final {
public:

    void draw(nonstd::span<SceneDecoration const> els, SceneRendererParams const& params)
    {
        m_Renderer.draw(els, params);

        // emit the texture to ImGui
        osc::DrawTextureAsImGuiImage(m_Renderer.updRenderTexture(), m_Renderer.getDimensions());
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
    SceneRenderer m_Renderer
    {
        App::config(),
        *App::singleton<osc::MeshCache>(),
        *App::singleton<osc::ShaderCache>(),
    };
    bool m_IsHovered = false;
    bool m_IsLeftClicked = false;
    bool m_IsRightClicked = false;
};


// public API (PIMPL)

osc::SceneViewer::SceneViewer() :
    m_Impl{new Impl{}}
{
}

osc::SceneViewer::SceneViewer(SceneViewer&&) noexcept = default;
osc::SceneViewer& osc::SceneViewer::operator=(SceneViewer&&) noexcept = default;
osc::SceneViewer::~SceneViewer() noexcept = default;

void osc::SceneViewer::draw(nonstd::span<SceneDecoration const> els, SceneRendererParams const& params)
{
    m_Impl->draw(els, params);
}

bool osc::SceneViewer::isHovered() const
{
    return m_Impl->isHovered();
}

bool osc::SceneViewer::isLeftClicked() const
{
    return m_Impl->isLeftClicked();
}

bool osc::SceneViewer::isRightClicked() const
{
    return m_Impl->isRightClicked();
}

#include "SceneViewer.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/SceneDecoration.hpp"
#include "src/Graphics/SceneRenderer.hpp"
#include "src/Graphics/SceneRendererParams.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Platform/App.hpp"

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
    SceneRenderer m_Renderer{osc::App::config(), osc::App::singleton<osc::MeshCache>(), osc::App::singleton<osc::ShaderCache>()};
    bool m_IsHovered = false;
    bool m_IsLeftClicked = false;
    bool m_IsRightClicked = false;
};


// public API (PIMPL)

osc::SceneViewer::SceneViewer() :
    m_Impl{new Impl{}}
{
}

osc::SceneViewer::~SceneViewer() noexcept = default;

void osc::SceneViewer::draw(nonstd::span<SceneDecoration const> els, SceneRendererParams const& params)
{
    m_Impl->draw(std::move(els), params);
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

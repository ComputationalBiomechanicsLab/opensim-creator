#include "UiModelViewer.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/IconCache.hpp"
#include "src/Graphics/MeshCache.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Graphics/SceneCollision.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/OpenSimBindings/Rendering/CachedModelRenderer.hpp"
#include "src/OpenSimBindings/Rendering/ModelRendererParams.hpp"
#include "src/OpenSimBindings/Widgets/BasicWidgets.hpp"
#include "src/OpenSimBindings/VirtualConstModelStatePair.hpp"
#include "src/Platform/App.hpp"
#include "src/Widgets/GuiRuler.hpp"

#include <imgui.h>

#include <memory>
#include <optional>

class osc::UiModelViewer::Impl final {
public:

    bool isLeftClicked() const
    {
        return m_RenderedImageHittest.isLeftClickReleasedWithoutDragging;
    }

    bool isRightClicked() const
    {
        return m_RenderedImageHittest.isRightClickReleasedWithoutDragging;
    }

    bool isMousedOver() const
    {
        return m_RenderedImageHittest.isHovered;
    }

    std::optional<SceneCollision> draw(VirtualConstModelStatePair const& rs)
    {
        // if this is the first frame being rendered, auto-focus the scene
        if (m_IsRenderingFirstFrame)
        {
            m_CachedModelRenderer.autoFocusCamera(
                rs,
                m_Params,
                AspectRatio(ImGui::GetContentRegionAvail())
            );
            m_IsRenderingFirstFrame = false;
        }

        // inputs: process inputs, if hovering
        if (m_RenderedImageHittest.isHovered)
        {
            UpdatePolarCameraFromImGuiUserInput(
                Dimensions(m_RenderedImageHittest.rect),
                m_Params.camera
            );

            UpdatePolarCameraFromKeyboardInputs(
                m_Params.camera,
                m_RenderedImageHittest.rect,
                m_CachedModelRenderer.getRootAABB()
            );
        }

        // render scene to texture
        m_CachedModelRenderer.draw(
            rs,
            m_Params,
            ImGui::GetContentRegionAvail(),
            App::get().getMSXAASamplesRecommended()
        );

        // blit texture as an ImGui::Image
        DrawTextureAsImGuiImage(
            m_CachedModelRenderer.updRenderTexture(),
            ImGui::GetContentRegionAvail()
        );
        m_RenderedImageHittest = osc::HittestLastImguiItem();

        // if allowed, hittest the scene
        std::optional<SceneCollision> hittestResult;
        if (m_RenderedImageHittest.isHovered && !IsDraggingWithAnyMouseButtonDown())
        {
            hittestResult = m_CachedModelRenderer.getClosestCollision(
                m_Params,
                ImGui::GetMousePos(),
                m_RenderedImageHittest.rect
            );
        }

        // draw 2D ImGui overlays
        DrawViewerImGuiOverlays(
            m_Params,
            m_CachedModelRenderer.getDrawlist(),
            m_CachedModelRenderer.getRootAABB(),
            m_RenderedImageHittest.rect,
            *m_IconCache,
            m_Ruler
        );

        // handle ruler and return value
        if (m_Ruler.isMeasuring())
        {
            std::optional<GuiRulerMouseHit> maybeHit;
            if (hittestResult)
            {
                maybeHit.emplace(hittestResult->decorationID, hittestResult->worldspaceLocation);
            }
            m_Ruler.draw(m_Params.camera, m_RenderedImageHittest.rect, maybeHit);

            // disable hittest while using the ruler
            return std::nullopt;
        }
        else
        {
            return hittestResult;
        }
    }

private:

    // rendering-related data
    ModelRendererParams m_Params;
    CachedModelRenderer m_CachedModelRenderer
    {
        App::get().getConfig(),
        App::singleton<MeshCache>(),
        *App::singleton<ShaderCache>(),
    };
    osc::ImGuiItemHittestResult m_RenderedImageHittest;

    // overlay-related data
    std::shared_ptr<IconCache> m_IconCache = osc::App::singleton<osc::IconCache>(
        App::resource("icons/"),
        ImGui::GetTextLineHeight()/128.0f
    );
    GuiRuler m_Ruler;

    // a flag that will auto-focus the main scene camera the next time it's used
    //
    // initialized `true`, so that the initially-loaded model is autofocused (#520)
    bool m_IsRenderingFirstFrame = true;
};


// public API (PIMPL)

osc::UiModelViewer::UiModelViewer() :
    m_Impl{std::make_unique<Impl>()}
{
}
osc::UiModelViewer::UiModelViewer(UiModelViewer&&) noexcept = default;
osc::UiModelViewer& osc::UiModelViewer::operator=(UiModelViewer&&) noexcept = default;
osc::UiModelViewer::~UiModelViewer() noexcept = default;

bool osc::UiModelViewer::isLeftClicked() const
{
    return m_Impl->isLeftClicked();
}

bool osc::UiModelViewer::isRightClicked() const
{
    return m_Impl->isRightClicked();
}

bool osc::UiModelViewer::isMousedOver() const
{
    return m_Impl->isMousedOver();
}

std::optional<osc::SceneCollision> osc::UiModelViewer::draw(VirtualConstModelStatePair const& rs)
{
    return m_Impl->draw(rs);
}

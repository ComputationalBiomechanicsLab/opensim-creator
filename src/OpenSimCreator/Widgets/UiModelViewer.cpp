#include "UiModelViewer.hpp"

#include "OpenSimCreator/Graphics/CachedModelRenderer.hpp"
#include "OpenSimCreator/Graphics/ModelRendererParams.hpp"
#include "OpenSimCreator/Model/VirtualConstModelStatePair.hpp"
#include "OpenSimCreator/Widgets/BasicWidgets.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Graphics/IconCache.hpp>
#include <oscar/Graphics/MeshCache.hpp>
#include <oscar/Graphics/ShaderCache.hpp>
#include <oscar/Graphics/SceneCollision.hpp>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Widgets/GuiRuler.hpp>
#include <oscar/Widgets/IconWithoutMenu.hpp>

#include <imgui.h>

#include <memory>
#include <optional>

class osc::UiModelViewer::Impl final {
public:

    bool isLeftClicked() const
    {
        return m_MaybeLastHittest && m_MaybeLastHittest->isLeftClickReleasedWithoutDragging;
    }

    bool isRightClicked() const
    {
        return m_MaybeLastHittest && m_MaybeLastHittest->isRightClickReleasedWithoutDragging;
    }

    bool isMousedOver() const
    {
        return m_MaybeLastHittest && m_MaybeLastHittest->isHovered;
    }

    std::optional<SceneCollision> onDraw(VirtualConstModelStatePair const& rs)
    {
        // if this is the first frame being rendered, auto-focus the scene
        if (!m_MaybeLastHittest)
        {
            m_CachedModelRenderer.autoFocusCamera(
                rs,
                m_Params,
                AspectRatio(ImGui::GetContentRegionAvail())
            );
        }

        // inputs: process inputs, if hovering
        if (m_MaybeLastHittest && m_MaybeLastHittest->isHovered)
        {
            UpdatePolarCameraFromImGuiInputs(
                m_Params.camera,
                m_MaybeLastHittest->rect,
                m_CachedModelRenderer.getRootAABB()
            );
        }

        // render scene to texture
        m_CachedModelRenderer.onDraw(
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

        // update current+retained hittest
        ImGuiItemHittestResult const hittest = osc::HittestLastImguiItem();
        m_MaybeLastHittest = hittest;

        // if allowed, hittest the scene
        std::optional<SceneCollision> hittestResult;
        if (hittest.isHovered && !IsDraggingWithAnyMouseButtonDown())
        {
            hittestResult = m_CachedModelRenderer.getClosestCollision(
                m_Params,
                ImGui::GetMousePos(),
                hittest.rect
            );
        }

        // draw 2D ImGui overlays
        DrawViewerImGuiOverlays(
            m_Params,
            m_CachedModelRenderer.getDrawlist(),
            m_CachedModelRenderer.getRootAABB(),
            hittest.rect,
            *m_IconCache,
            [this]() { drawRulerButton(); }
        );

        // handle ruler and return value
        if (m_Ruler.isMeasuring())
        {
            m_Ruler.onDraw(m_Params.camera, hittest.rect, hittestResult);
            return std::nullopt;  // disable hittest while measuring
        }
        else
        {
            return hittestResult;
        }
    }

    std::optional<osc::Rect> getScreenRect() const
    {
        return m_MaybeLastHittest ?
            std::optional<osc::Rect>{m_MaybeLastHittest->rect} :
            std::nullopt;
    }

private:
    void drawRulerButton()
    {
        IconWithoutMenu rulerButton
        {
            m_IconCache->getIcon("ruler"),
            "Ruler",
            "Roughly measure something in the scene",
        };
        if (rulerButton.onDraw())
        {
            m_Ruler.toggleMeasuring();
        }
    }

    // rendering-related data
    ModelRendererParams m_Params;
    CachedModelRenderer m_CachedModelRenderer
    {
        App::get().getConfig(),
        App::singleton<MeshCache>(),
        *App::singleton<ShaderCache>(),
    };

    // only available after rendering the first frame
    std::optional<ImGuiItemHittestResult> m_MaybeLastHittest;

    // overlay-related data
    std::shared_ptr<IconCache> m_IconCache = App::singleton<osc::IconCache>(
        App::resource("icons/"),
        ImGui::GetTextLineHeight()/128.0f
    );
    GuiRuler m_Ruler;
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

std::optional<osc::SceneCollision> osc::UiModelViewer::onDraw(VirtualConstModelStatePair const& rs)
{
    return m_Impl->onDraw(rs);
}

std::optional<osc::Rect> osc::UiModelViewer::getScreenRect() const
{
    return m_Impl->getScreenRect();
}

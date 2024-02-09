#include "Readonly3DModelViewer.hpp"

#include <OpenSimCreator/Graphics/CachedModelRenderer.hpp>
#include <OpenSimCreator/Graphics/ModelRendererParams.hpp>
#include <OpenSimCreator/UI/Shared/BasicWidgets.hpp>

#include <imgui.h>
#include <oscar/Maths/MathHelpers.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Scene/SceneCache.hpp>
#include <oscar/Scene/SceneCollision.hpp>
#include <oscar/Scene/ShaderCache.hpp>
#include <oscar/UI/IconCache.hpp>
#include <oscar/UI/ImGuiHelpers.hpp>
#include <oscar/UI/Widgets/GuiRuler.hpp>
#include <oscar/UI/Widgets/IconWithoutMenu.hpp>

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using namespace osc;

namespace
{
    std::string GetSettingsKeyPrefixForPanel(std::string_view panelName)
    {
        std::stringstream ss;
        ss << "panels/" << panelName << '/';
        return std::move(ss).str();
    }
}

class osc::Readonly3DModelViewer::Impl final {
public:
    explicit Impl(std::string_view parentPanelName_) :
        m_ParentPanelName{parentPanelName_}
    {
        UpdModelRendererParamsFrom(
            App::config(),
            GetSettingsKeyPrefixForPanel(parentPanelName_),
            m_Params
        );
    }

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

    std::optional<SceneCollision> onDraw(IConstModelStatePair const& rs)
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
            App::get().getCurrentAntiAliasingLevel()
        );

        // blit texture as an ImGui::Image
        DrawTextureAsImGuiImage(
            m_CachedModelRenderer.updRenderTexture(),
            ImGui::GetContentRegionAvail()
        );

        // update current+retained hittest
        ImGuiItemHittestResult const hittest = HittestLastImguiItem();
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
        auto renderParamsBefore = m_Params;
        bool const edited = DrawViewerImGuiOverlays(
            m_Params,
            m_CachedModelRenderer.getDrawlist(),
            m_CachedModelRenderer.getRootAABB(),
            hittest.rect,
            *m_IconCache,
            [this]() { return drawRulerButton(); }
        );
        if (edited)
        {
            auto const& renderParamsAfter = m_Params;
            SaveModelRendererParamsDifference(
                renderParamsBefore,
                renderParamsAfter,
                GetSettingsKeyPrefixForPanel(m_ParentPanelName),
                App::upd().updConfig()
            );
        }

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

    std::optional<Rect> getScreenRect() const
    {
        return m_MaybeLastHittest ?
            std::optional<Rect>{m_MaybeLastHittest->rect} :
            std::nullopt;
    }

private:
    bool drawRulerButton()
    {
        IconWithoutMenu rulerButton
        {
            m_IconCache->getIcon("ruler"),
            "Ruler",
            "Roughly measure something in the scene",
        };

        bool rv = false;
        if (rulerButton.onDraw())
        {
            m_Ruler.toggleMeasuring();
            rv = true;
        }
        return rv;
    }

    // used for saving per-panel data to the application config
    std::string m_ParentPanelName;

    // rendering-related data
    ModelRendererParams m_Params;
    CachedModelRenderer m_CachedModelRenderer
    {
        App::singleton<SceneCache>(),
        *App::singleton<ShaderCache>(App::resource_loader()),
    };

    // only available after rendering the first frame
    std::optional<ImGuiItemHittestResult> m_MaybeLastHittest;

    // overlay-related data
    std::shared_ptr<IconCache> m_IconCache = App::singleton<IconCache>(
        App::resource_loader().withPrefix("icons/"),
        ImGui::GetTextLineHeight()/128.0f
    );
    GuiRuler m_Ruler;
};


// public API (PIMPL)

osc::Readonly3DModelViewer::Readonly3DModelViewer(std::string_view parentPanelName_) :
    m_Impl{std::make_unique<Impl>(parentPanelName_)}
{
}
osc::Readonly3DModelViewer::Readonly3DModelViewer(Readonly3DModelViewer&&) noexcept = default;
osc::Readonly3DModelViewer& osc::Readonly3DModelViewer::operator=(Readonly3DModelViewer&&) noexcept = default;
osc::Readonly3DModelViewer::~Readonly3DModelViewer() noexcept = default;

bool osc::Readonly3DModelViewer::isLeftClicked() const
{
    return m_Impl->isLeftClicked();
}

bool osc::Readonly3DModelViewer::isRightClicked() const
{
    return m_Impl->isRightClicked();
}

bool osc::Readonly3DModelViewer::isMousedOver() const
{
    return m_Impl->isMousedOver();
}

std::optional<SceneCollision> osc::Readonly3DModelViewer::onDraw(IConstModelStatePair const& rs)
{
    return m_Impl->onDraw(rs);
}

std::optional<Rect> osc::Readonly3DModelViewer::getScreenRect() const
{
    return m_Impl->getScreenRect();
}

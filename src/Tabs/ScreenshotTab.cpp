#include "ScreenshotTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/AnnotatedImage.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/Geometry.hpp"
#include "src/Platform/os.hpp"

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <filesystem>
#include <future>
#include <string>
#include <utility>

namespace
{
    // returns a rect that fully spans at least one dimension of the target rect, but has
    // the given aspect ratio
    //
    // the returned rectangle is in the same space as the target rectangle
    osc::Rect ShrinkToFit(osc::Rect targetRect, float aspectRatio)
    {
        float const targetAspectRatio = osc::AspectRatio(targetRect);
        float const ratio = targetAspectRatio / aspectRatio;
        glm::vec2 const targetDims = osc::Dimensions(targetRect);

        if (ratio >= 1.0f)
        {
            // it will touch the top/bottom but may (ratio != 1.0f) fall short of the left/right
            glm::vec2 const rvDims = {targetDims.x/ratio, targetDims.y};
            glm::vec2 const rvTopLeft = {targetRect.p1.x + 0.5f*(targetDims.x - rvDims.x), targetRect.p1.y};
            return {rvTopLeft, rvTopLeft + rvDims};
        }
        else
        {
            // it will touch the left/right but will not touch the top/bottom
            glm::vec2 const rvDims = {targetDims.x, ratio*targetDims.y};
            glm::vec2 const rvTopLeft = {targetRect.p1.x, targetRect.p1.y + 0.5f*(targetDims.y - rvDims.y)};
            return {rvTopLeft, rvTopLeft + rvDims};
        }
    }

    osc::Rect MapRect(osc::Rect const& sourceRect, osc::Rect const& targetRect, osc::Rect const& rect)
    {
        glm::vec2 const scale = osc::Dimensions(targetRect) / osc::Dimensions(sourceRect);

        return osc::Rect
        {
            targetRect.p1 + scale*(rect.p1 - sourceRect.p1),
            targetRect.p1 + scale*(rect.p2 - sourceRect.p1),
        };
    }
}

class osc::ScreenshotTab::Impl final {
public:

    Impl(TabHost* parent, AnnotatedImage&& annotatedImage) :
        m_Parent{std::move(parent)},
        m_AnnotatedImage{std::move(annotatedImage)}
    {
        m_ImageTexture.setFilterMode(osc::TextureFilterMode::Mipmap);
    }

    UID getID() const
    {
        return m_ID;
    }

    CStringView getName() const
    {
        return m_Name;
    }

    TabHost* parent()
    {
        return m_Parent;
    }

    void onMount()
    {

    }

    void onUnmount()
    {

    }

    bool onEvent(SDL_Event const&)
    {
        return false;
    }

    void onTick()
    {

    }

    void onDrawMainMenu()
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save"))
            {
                std::filesystem::path maybePath = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("png");
                if (!maybePath.empty())
                {
                    osc::WriteToPNG(m_AnnotatedImage.image, maybePath);
                    osc::log::info("screenshot: created: path = %s, dimensions = (%i, %i)", maybePath.string().c_str(), m_AnnotatedImage.image.getDimensions().x, m_AnnotatedImage.image.getDimensions().y);
                }
            }
            ImGui::EndMenu();
        }
    }

    void onDraw()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        // draw screenshot window
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
            ImGui::Begin("Screenshot");
            ImGui::PopStyleVar();

            // draw the screenshot
            glm::vec2 const screenTopLeft = ImGui::GetCursorScreenPos();
            Rect const windowRect = {screenTopLeft, screenTopLeft + glm::vec2{ImGui::GetContentRegionAvail()}};
            Rect const imageRect = ShrinkToFit(windowRect, osc::AspectRatio(m_AnnotatedImage.image.getDimensions()));
            ImGui::SetCursorScreenPos(imageRect.p1);
            DrawTextureAsImGuiImage(m_ImageTexture, osc::Dimensions(imageRect));

            // draw overlays
            Rect const imageSourceRect = {{0.0f, 0.0f}, m_AnnotatedImage.image.getDimensions()};
            ImDrawList* drawlist = ImGui::GetWindowDrawList();
            ImU32 const red = ImGui::ColorConvertFloat4ToU32({1.0f, 0.0f, 0.0f, 1.0f});

            for (AnnotatedImage::Annotation const& annotation : m_AnnotatedImage.annotations)
            {
                Rect const r = MapRect(imageSourceRect, imageRect, annotation.rect);                
                drawlist->AddRect(r.p1, r.p2, red, 3.0f, 0, 3.0f);
            }
            ImGui::End();
        }

        // draw controls window
        {
            int id = 0;
            ImGui::Begin("Controls");
            for (AnnotatedImage::Annotation const& annotation : m_AnnotatedImage.annotations)
            {
                ImGui::PushID(id++);
                ImGui::Text(annotation.label.c_str());
                ImGui::PopID();
            }
            ImGui::End();
        }
    }

private:
    UID m_ID;
    std::string m_Name = ICON_FA_COOKIE " ScreenshotTab";
    TabHost* m_Parent;
    AnnotatedImage m_AnnotatedImage;
    Texture2D m_ImageTexture{m_AnnotatedImage.image.getDimensions().x, m_AnnotatedImage.image.getDimensions().y, m_AnnotatedImage.image.getPixelData(), m_AnnotatedImage.image.getNumChannels()};
};


// public API

osc::ScreenshotTab::ScreenshotTab(TabHost* parent, AnnotatedImage&& image) :
    m_Impl{new Impl{std::move(parent), std::move(image)}}
{
}

osc::ScreenshotTab::ScreenshotTab(ScreenshotTab&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::ScreenshotTab& osc::ScreenshotTab::operator=(ScreenshotTab&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::ScreenshotTab::~ScreenshotTab() noexcept
{
    delete m_Impl;
}

osc::UID osc::ScreenshotTab::implGetID() const
{
    return m_Impl->getID();
}

osc::CStringView osc::ScreenshotTab::implGetName() const
{
    return m_Impl->getName();
}

osc::TabHost* osc::ScreenshotTab::implParent() const
{
    return m_Impl->parent();
}

void osc::ScreenshotTab::implOnMount()
{
    m_Impl->onMount();
}

void osc::ScreenshotTab::implOnUnmount()
{
    m_Impl->onUnmount();
}

bool osc::ScreenshotTab::implOnEvent(SDL_Event const& e)
{
    return m_Impl->onEvent(e);
}

void osc::ScreenshotTab::implOnTick()
{
    m_Impl->onTick();
}

void osc::ScreenshotTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ScreenshotTab::implOnDraw()
{
    m_Impl->onDraw();
}

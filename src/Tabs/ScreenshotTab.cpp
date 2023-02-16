#include "ScreenshotTab.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/Graphics/AnnotatedImage.hpp"
#include "src/Graphics/Camera.hpp"
#include "src/Graphics/Color.hpp"
#include "src/Graphics/Graphics.hpp"
#include "src/Graphics/Material.hpp"
#include "src/Graphics/RenderTexture.hpp"
#include "src/Graphics/Shader.hpp"
#include "src/Graphics/ShaderCache.hpp"
#include "src/Graphics/Mesh.hpp"
#include "src/Graphics/Texture2D.hpp"
#include "src/Maths/CollisionTests.hpp"
#include "src/Maths/MathHelpers.hpp"
#include "src/Platform/App.hpp"
#include "src/Platform/os.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Utils/Assertions.hpp"

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <filesystem>
#include <future>
#include <string>
#include <utility>
#include <unordered_set>

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

    osc::Texture2D ToTexture(osc::Image const& img)
    {
        return osc::Texture2D
        {
            img.getDimensions(),
            img.getPixelData(),
            img.getNumChannels(),
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
                actionSaveOutputImage();
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

            Rect imageRect = drawScreenshot();
            drawOverlays(*ImGui::GetWindowDrawList(), imageRect, m_UnselectedColor, m_SelectedColor);

            ImGui::End();
        }

        // draw controls window
        {
            int id = 0;
            ImGui::Begin("Controls");
            for (ImageAnnotation const& annotation : m_AnnotatedImage.annotations)
            {
                ImGui::PushID(id++);
                ImGui::TextUnformatted(annotation.label.c_str());
                ImGui::PopID();
            }
            ImGui::End();
        }
    }

private:

    // returns screenspace rect of the screenshot within the UI
    Rect drawScreenshot()
    {
        glm::vec2 const screenTopLeft = ImGui::GetCursorScreenPos();
        Rect const windowRect = {screenTopLeft, screenTopLeft + glm::vec2{ImGui::GetContentRegionAvail()}};
        Rect const imageRect = ShrinkToFit(windowRect, osc::AspectRatio(m_AnnotatedImage.image.getDimensions()));
        ImGui::SetCursorScreenPos(imageRect.p1);
        DrawTextureAsImGuiImage(m_ImageTexture, osc::Dimensions(imageRect));
        return imageRect;
    }

    void drawOverlays(ImDrawList& drawlist, Rect const& imageRect, glm::vec4 const& unselectedColor, glm::vec4 const& selectedColor)
    {
        glm::vec2 const mousePos = ImGui::GetMousePos();
        bool const leftClickReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
        Rect const imageSourceRect = {{0.0f, 0.0f}, m_AnnotatedImage.image.getDimensions()};

        for (ImageAnnotation const& annotation : m_AnnotatedImage.annotations)
        {
            Rect const annotationRectScreenSpace = MapRect(imageSourceRect, imageRect, annotation.rect);
            bool const selected =  Contains(m_SelectedAnnotations, annotation.label);
            bool const hovered = osc::IsPointInRect(annotationRectScreenSpace, mousePos);

            glm::vec4 color = selected ? selectedColor : unselectedColor;
            if (hovered)
            {
                color.a = glm::clamp(color.a + 0.3f, 0.0f, 1.0f);
            }

            if (hovered && leftClickReleased)
            {
                if (selected)
                {
                    m_SelectedAnnotations.erase(annotation.label);
                }
                else
                {
                    m_SelectedAnnotations.insert(annotation.label);
                }
            }

            drawlist.AddRect(
                annotationRectScreenSpace.p1,
                annotationRectScreenSpace.p2,
                ImGui::ColorConvertFloat4ToU32(color),
                3.0f,
                0,
                3.0f
            );
        }
    }

    void actionSaveOutputImage()
    {
        std::optional<std::filesystem::path> const maybeImagePath = osc::PromptUserForFileSaveLocationAndAddExtensionIfNecessary("png");
        if (maybeImagePath)
        {
            Image outputImage = renderOutputImage();
            osc::WriteImageToPNGFile(outputImage, *maybeImagePath);
            osc::OpenPathInOSDefaultApplication(*maybeImagePath);
        }
    }

    Image renderOutputImage()
    {
        std::optional<RenderTexture> rt;
        rt.emplace(RenderTextureDescriptor{m_ImageTexture.getDimensions()});

        // blit the screenshot into the output
        Graphics::Blit(m_ImageTexture, *rt);

        // draw overlays to a local ImGui drawlist
        ImDrawList drawlist{ImGui::GetDrawListSharedData()};
        drawlist.Flags |= ImDrawListFlags_AntiAliasedLines;
        drawlist.AddDrawCmd();
        glm::vec4 outlineColor = m_SelectedColor;
        outlineColor.a = 1.0f;
        drawOverlays(
            drawlist,
            Rect{{0.0f, 0.0f}, m_ImageTexture.getDimensions()},
            {0.0f, 0.0f, 0.0f, 0.0f},
            outlineColor
        );

        // render drawlist to output
        {
            // upload vertex positions/colors
            Mesh mesh;
            {
                // verts
                {
                    std::vector<glm::vec3> verts;
                    verts.reserve(drawlist.VtxBuffer.size());
                    for (int i = 0; i < drawlist.VtxBuffer.size(); ++i)
                    {
                        verts.push_back(glm::vec3{glm::vec2{drawlist.VtxBuffer[i].pos}, 0.0f});
                    }
                    mesh.setVerts(verts);
                }

                // colors
                {
                    std::vector<Rgba32> colors;
                    colors.reserve(drawlist.VtxBuffer.size());
                    for (int i = 0; i < drawlist.VtxBuffer.size(); ++i)
                    {
                        ImU32 colorImgui = drawlist.VtxBuffer[i].col;
                        glm::vec4 linearColor = ImGui::ColorConvertU32ToFloat4(colorImgui);
                        colors.push_back(ToRgba32(linearColor));
                    }
                    mesh.setColors(colors);
                }
            }

            // solid color material
            Material material
            {
                Shader
                {
                    App::slurp("shaders/PerVertexColor.vert"),
                    App::slurp("shaders/PerVertexColor.frag"),
                }
            };

            Camera c;
            c.setViewMatrixOverride(glm::mat4{1.0f});

            {
                // project screenspace overlays into NDC
                float L = 0.0f;
                float R = static_cast<float>(m_ImageTexture.getDimensions().x);
                float T = 0.0f;
                float B = static_cast<float>(m_ImageTexture.getDimensions().y);
                glm::mat4 const proj =
                {
                    { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
                    { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
                    { 0.0f,         0.0f,        -1.0f,   0.0f },
                    { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
                };
                c.setProjectionMatrixOverride(proj);
            }
            c.setClearFlags(CameraClearFlags::Nothing);

            for (int cmdIdx = 0; cmdIdx < drawlist.CmdBuffer.Size; ++cmdIdx)
            {
                ImDrawCmd const& cmd = drawlist.CmdBuffer[cmdIdx];
                {
                    // upload indices
                    std::vector<ImDrawIdx> indices;
                    indices.reserve(cmd.ElemCount);
                    for (unsigned int i = cmd.IdxOffset; i < cmd.IdxOffset + cmd.ElemCount; ++i)
                    {
                        indices.push_back(drawlist.IdxBuffer[static_cast<int>(i)]);
                    }
                    mesh.setIndices(indices);
                }
                Graphics::DrawMesh(mesh, Transform{}, material, c);
            }

            OSC_ASSERT(rt.has_value());
            c.renderTo(*rt);
        }

        Image rv;
        Graphics::ReadPixels(*rt, rv);
        return rv;
    }

    UID m_ID;
    std::string m_Name = ICON_FA_COOKIE " ScreenshotTab";
    TabHost* m_Parent;
    AnnotatedImage m_AnnotatedImage;
    Texture2D m_ImageTexture = ToTexture(m_AnnotatedImage.image);
    std::unordered_set<std::string> m_SelectedAnnotations;
    glm::vec4 m_UnselectedColor = {1.0f, 1.0f, 1.0f, 0.4f};
    glm::vec4 m_SelectedColor = {1.0f, 0.0f, 0.0f, 0.8f};
};


// public API

osc::ScreenshotTab::ScreenshotTab(TabHost* parent, AnnotatedImage&& image) :
    m_Impl{std::make_unique<Impl>(std::move(parent), std::move(image))}
{
}

osc::ScreenshotTab::ScreenshotTab(ScreenshotTab&&) noexcept = default;
osc::ScreenshotTab& osc::ScreenshotTab::operator=(ScreenshotTab&&) noexcept = default;
osc::ScreenshotTab::~ScreenshotTab() noexcept = default;

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

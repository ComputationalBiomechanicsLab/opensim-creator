#include "ScreenshotTab.hpp"

#include "oscar/Bindings/ImGuiHelpers.hpp"
#include "oscar/Graphics/AnnotatedImage.hpp"
#include "oscar/Graphics/Camera.hpp"
#include "oscar/Graphics/Color.hpp"
#include "oscar/Graphics/Graphics.hpp"
#include "oscar/Graphics/GraphicsHelpers.hpp"
#include "oscar/Graphics/Material.hpp"
#include "oscar/Graphics/RenderTexture.hpp"
#include "oscar/Graphics/Shader.hpp"
#include "oscar/Graphics/ShaderCache.hpp"
#include "oscar/Graphics/Mesh.hpp"
#include "oscar/Graphics/Texture2D.hpp"
#include "oscar/Maths/CollisionTests.hpp"
#include "oscar/Maths/MathHelpers.hpp"
#include "oscar/Platform/App.hpp"
#include "oscar/Platform/os.hpp"
#include "oscar/Utils/Assertions.hpp"
#include "oscar/Utils/SetHelpers.hpp"

#include <IconsFontAwesome5.h>
#include <SDL_events.h>

#include <filesystem>
#include <future>
#include <string>
#include <utility>
#include <unordered_set>

namespace
{
    constexpr osc::Color c_UnselectedColor = {1.0f, 1.0f, 1.0f, 0.4f};
    constexpr osc::Color c_SelectedColor = {1.0f, 0.0f, 0.0f, 0.8f};

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

    explicit Impl(AnnotatedImage&& annotatedImage) :
        m_AnnotatedImage{std::move(annotatedImage)}
    {
        m_ImageTexture.setFilterMode(TextureFilterMode::Mipmap);
    }

    UID getID() const
    {
        return m_TabID;
    }

    CStringView getName() const
    {
        return ICON_FA_COOKIE " ScreenshotTab";
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
            drawOverlays(*ImGui::GetWindowDrawList(), imageRect, c_UnselectedColor, c_SelectedColor);

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

    void drawOverlays(
        ImDrawList& drawlist,
        Rect const& imageRect,
        Color const& unselectedColor,
        Color const& selectedColor)
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
        Color outlineColor = c_SelectedColor;
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
                    for (ImDrawVert const& vert : drawlist.VtxBuffer)
                    {
                        verts.emplace_back(vert.pos.x, vert.pos.y, 0.0f);
                    }
                    mesh.setVerts(verts);
                }

                // colors
                {
                    std::vector<Rgba32> colors;
                    colors.reserve(drawlist.VtxBuffer.size());
                    for (ImDrawVert const& vert : drawlist.VtxBuffer)
                    {
                        ImU32 const colorImgui = vert.col;
                        glm::vec4 const linearColor = ImGui::ColorConvertU32ToFloat4(colorImgui);
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

    UID m_TabID;

    AnnotatedImage m_AnnotatedImage;
    Texture2D m_ImageTexture = ToTexture2D(m_AnnotatedImage.image);
    std::unordered_set<std::string> m_SelectedAnnotations;
};


// public API

osc::ScreenshotTab::ScreenshotTab(std::weak_ptr<TabHost>, AnnotatedImage&& image) :
    m_Impl{std::make_unique<Impl>(std::move(image))}
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

void osc::ScreenshotTab::implOnDrawMainMenu()
{
    m_Impl->onDrawMainMenu();
}

void osc::ScreenshotTab::implOnDraw()
{
    m_Impl->onDraw();
}

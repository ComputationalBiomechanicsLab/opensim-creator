#pragma once

#include <OpenSimCreator/Documents/MeshImporter/Document.h>
#include <OpenSimCreator/Documents/MeshImporter/Mesh.h>
#include <OpenSimCreator/UI/MeshImporter/DrawableThing.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterHover.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterSharedState.h>
#include <OpenSimCreator/UI/MeshImporter/MeshImporterUILayer.h>

#include <oscar/Maths/Vec2.h>
#include <oscar/Maths/Vec3.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/CStringView.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// select 2 mesh points layer
namespace osc::mi
{
    // runtime options for "Select two mesh points" UI layer
    struct Select2MeshPointsOptions final {

        // a function that is called when the implementation detects two points have
        // been clicked
        //
        // the function should return `true` if the points are accepted
        std::function<bool(Vec3, Vec3)> onTwoPointsChosen = [](Vec3, Vec3)
        {
            return true;
        };

        std::string header = "choose first (left-click) and second (right click) mesh positions (ESC to cancel)";
    };

    // UI layer that lets the user select two points on a mesh with left-click and
    // right-click
    class Select2MeshPointsLayer final : public MeshImporterUILayer {
    public:
        Select2MeshPointsLayer(
            IMeshImporterUILayerHost& parent,
            std::shared_ptr<MeshImporterSharedState> shared,
            Select2MeshPointsOptions options) :

            MeshImporterUILayer{parent},
            m_Shared{std::move(shared)},
            m_Options{std::move(options)}
        {
        }

    private:

        bool isBothPointsSelected() const
        {
            return m_MaybeFirstLocation && m_MaybeSecondLocation;
        }

        bool isAnyPointSelected() const
        {
            return m_MaybeFirstLocation || m_MaybeSecondLocation;
        }

        // handle the transition that may occur after the user clicks two points
        void handlePossibleTransitionToNextStep()
        {
            if (!isBothPointsSelected())
            {
                return;  // user hasn't selected two points yet
            }

            bool pointsAccepted = m_Options.onTwoPointsChosen(*m_MaybeFirstLocation, *m_MaybeSecondLocation);

            if (pointsAccepted)
            {
                requestPop();
            }
            else
            {
                // points were rejected, so reset them
                m_MaybeFirstLocation.reset();
                m_MaybeSecondLocation.reset();
            }
        }

        // handle any side-effects of the user interacting with whatever they are
        // hovered over
        void handleHovertestSideEffects()
        {
            if (!m_MaybeCurrentHover)
            {
                return;  // nothing hovered
            }
            else if (ui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                // LEFT CLICK: set first mouse location
                m_MaybeFirstLocation = m_MaybeCurrentHover.Pos;
                handlePossibleTransitionToNextStep();
            }
            else if (ui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                // RIGHT CLICK: set second mouse location
                m_MaybeSecondLocation = m_MaybeCurrentHover.Pos;
                handlePossibleTransitionToNextStep();
            }
        }

        // generate 3D drawable geometry for this particular layer
        std::vector<DrawableThing>& generateDrawables()
        {
            m_DrawablesBuffer.clear();

            Document const& mg = m_Shared->getModelGraph();

            for (Mesh const& meshEl : mg.iter<Mesh>())
            {
                m_DrawablesBuffer.emplace_back(m_Shared->generateMeshDrawable(meshEl));
            }

            m_DrawablesBuffer.push_back(m_Shared->generateFloorDrawable());

            return m_DrawablesBuffer;
        }

        // draw tooltip that pops up when user is moused over a mesh
        void drawHoverTooltip()
        {
            if (!m_MaybeCurrentHover)
            {
                return;
            }

            // returns a string representation of a spatial position (e.g. (0.0, 1.0, 3.0))
            std::string const pos = [](Vec3 const& pos)
            {
                std::stringstream ss;
                ss.precision(4);
                ss << '(' << pos.x << ", " << pos.y << ", " << pos.z << ')';
                return std::move(ss).str();
            }(m_MaybeCurrentHover.Pos);

            ImGui::BeginTooltip();
            ui::Text(pos);
            ui::TextDisabled("(left-click to assign as first point, right-click to assign as second point)");
            ImGui::EndTooltip();
        }

        // draw 2D overlay over the render, things like connection lines, dots, etc.
        void drawOverlay()
        {
            if (!isAnyPointSelected())
            {
                return;
            }

            Vec3 clickedWorldPos = m_MaybeFirstLocation ? *m_MaybeFirstLocation : *m_MaybeSecondLocation;
            Vec2 clickedScrPos = m_Shared->worldPosToScreenPos(clickedWorldPos);

            auto color = ToImU32(Color::black());

            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddCircleFilled(clickedScrPos, 5.0f, color);

            if (!m_MaybeCurrentHover) {
                return;
            }

            Vec2 hoverScrPos = m_Shared->worldPosToScreenPos(m_MaybeCurrentHover.Pos);

            dl->AddCircleFilled(hoverScrPos, 5.0f, color);
            dl->AddLine(clickedScrPos, hoverScrPos, color, 5.0f);
        }

        // draw 2D "choose something" text at the top of the render
        void drawHeaderText() const
        {
            if (m_Options.header.empty())
            {
                return;
            }

            ImU32 color = ToImU32(Color::white());
            Vec2 padding{10.0f, 10.0f};
            Vec2 pos = m_Shared->get3DSceneRect().p1 + padding;
            ImGui::GetWindowDrawList()->AddText(pos, color, m_Options.header.c_str());
        }

        // draw a user-clickable button for cancelling out of this choosing state
        void drawCancelButton()
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10.0f, 10.0f});
            PushStyleColor(ImGuiCol_Button, Color::halfGrey());

            CStringView const text = ICON_FA_ARROW_LEFT " Cancel (ESC)";
            Vec2 const margin = {25.0f, 35.0f};
            Vec2 const buttonTopLeft = m_Shared->get3DSceneRect().p2 - (CalcButtonSize(text) + margin);

            ImGui::SetCursorScreenPos(buttonTopLeft);
            if (ui::Button(text.c_str()))
            {
                requestPop();
            }

            PopStyleColor();
            ImGui::PopStyleVar();
        }

        bool implOnEvent(SDL_Event const& e) final
        {
            return m_Shared->onEvent(e);
        }

        void implTick(float dt) final
        {
            m_Shared->tick(dt);

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                // ESC: user cancelled out
                requestPop();
            }

            bool isRenderHovered = m_Shared->isRenderHovered();

            if (isRenderHovered)
            {
                UpdatePolarCameraFromImGuiMouseInputs(m_Shared->updCamera(), m_Shared->get3DSceneDims());
            }
        }

        void implOnDraw() final
        {
            m_Shared->setContentRegionAvailAsSceneRect();
            std::vector<DrawableThing>& drawables = generateDrawables();
            m_MaybeCurrentHover = m_Shared->doHovertest(drawables);
            handleHovertestSideEffects();

            m_Shared->drawScene(drawables);
            drawOverlay();
            drawHoverTooltip();
            drawHeaderText();
            drawCancelButton();
        }

        // data that's shared between other UI states
        std::shared_ptr<MeshImporterSharedState> m_Shared;

        // options for this state
        Select2MeshPointsOptions m_Options;

        // (maybe) user mouse hover
        MeshImporterHover m_MaybeCurrentHover;

        // (maybe) first mesh location
        std::optional<Vec3> m_MaybeFirstLocation;

        // (maybe) second mesh location
        std::optional<Vec3> m_MaybeSecondLocation;

        // buffer that's filled with drawable geometry during a drawcall
        std::vector<DrawableThing> m_DrawablesBuffer;
    };
}

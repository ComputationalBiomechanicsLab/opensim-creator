#pragma once

#include <OpenSimCreator/Documents/ModelGraph/ModelGraph.hpp>
#include <OpenSimCreator/Documents/ModelGraph/ModelGraphIDs.hpp>
#include <OpenSimCreator/Documents/ModelGraph/SceneEl.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/DrawableThing.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/MeshImporterHover.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/MeshImporterSharedState.hpp>
#include <OpenSimCreator/UI/Tabs/MeshImporter/MeshImporterUILayer.hpp>

#include <imgui.h>
#include <oscar/Graphics/Color.hpp>
#include <oscar/Maths/EasingFunctions.hpp>
#include <oscar/Maths/Vec2.hpp>
#include <oscar/Maths/Vec3.hpp>
#include <oscar/Platform/App.hpp>
#include <oscar/Scene/SceneDecorationFlags.hpp>
#include <oscar/Utils/CStringView.hpp>
#include <oscar/Utils/UID.hpp>
#include <SDL_events.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

// choose specific element layer
namespace osc
{
    // options for when the UI transitions into "choose something" mode
    struct ChooseElLayerOptions final {

        // types of elements the user can choose in this screen
        bool canChooseBodies = true;
        bool canChooseGround = true;
        bool canChooseMeshes = true;
        bool canChooseJoints = true;
        bool canChooseStations = false;

        // (maybe) elements the assignment is ultimately assigning
        std::unordered_set<UID> maybeElsAttachingTo;

        // false implies the user is attaching "away from" what they select (used for drawing arrows)
        bool isAttachingTowardEl = true;

        // (maybe) elements that are being replaced by the user's choice
        std::unordered_set<UID> maybeElsBeingReplacedByChoice;

        // the number of elements the user must click before OnUserChoice is called
        int numElementsUserMustChoose = 1;

        // function that returns true if the "caller" is happy with the user's choice
        std::function<bool(std::span<UID>)> onUserChoice = [](std::span<UID>)
        {
            return true;
        };

        // user-facing header text
        std::string header = "choose something";
    };

    // "choose `n` things" UI layer
    //
    // this is what's drawn when the user's being prompted to choose scene elements
    class ChooseElLayer final : public MeshImporterUILayer {
    public:
        ChooseElLayer(
            MeshImporterUILayerHost& parent,
            std::shared_ptr<MeshImporterSharedState> shared,
            ChooseElLayerOptions options) :

            MeshImporterUILayer{parent},
            m_Shared{std::move(shared)},
            m_Options{std::move(options)}
        {
        }

    private:
        // returns true if the user's mouse is hovering over the given scene element
        bool isHovered(SceneEl const& el) const
        {
            return el.getID() == m_MaybeHover.ID;
        }

        // returns true if the user has already selected the given scene element
        bool isSelected(SceneEl const& el) const
        {
            return std::find(m_SelectedEls.begin(), m_SelectedEls.end(), el.getID()) != m_SelectedEls.end();
        }

        // returns true if the user can (de)select the given element
        bool isSelectable(SceneEl const& el) const
        {
            if (m_Options.maybeElsAttachingTo.find(el.getID()) != m_Options.maybeElsAttachingTo.end())
            {
                return false;
            }

            return std::visit(Overload
            {
                [this](GroundEl const&)  { return m_Options.canChooseGround; },
                [this](MeshEl const&)    { return m_Options.canChooseMeshes; },
                [this](BodyEl const&)    { return m_Options.canChooseBodies; },
                [this](JointEl const&)   { return m_Options.canChooseJoints; },
                [this](StationEl const&) { return m_Options.canChooseStations; },
            }, el.toVariant());
        }

        void select(SceneEl const& el)
        {
            if (!isSelectable(el))
            {
                return;
            }

            if (isSelected(el))
            {
                return;
            }

            m_SelectedEls.push_back(el.getID());
        }

        void deSelect(SceneEl const& el)
        {
            if (!isSelectable(el))
            {
                return;
            }

            std::erase_if(m_SelectedEls, [elID = el.getID()](UID id) { return id == elID; } );
        }

        void tryToggleSelectionStateOf(SceneEl const& el)
        {
            isSelected(el) ? deSelect(el) : select(el);
        }

        void tryToggleSelectionStateOf(UID id)
        {
            SceneEl const* el = m_Shared->getModelGraph().tryGetElByID(id);

            if (el)
            {
                tryToggleSelectionStateOf(*el);
            }
        }

        SceneDecorationFlags computeFlags(SceneEl const& el) const
        {
            if (isSelected(el))
            {
                return SceneDecorationFlags::IsSelected;
            }
            else if (isHovered(el))
            {
                return SceneDecorationFlags::IsHovered;
            }
            else
            {
                return SceneDecorationFlags::None;
            }
        }

        // returns a list of 3D drawable scene objects for this layer
        std::vector<DrawableThing>& generateDrawables()
        {
            m_DrawablesBuffer.clear();

            ModelGraph const& mg = m_Shared->getModelGraph();

            float fadedAlpha = 0.2f;
            float animScale = osc::EaseOutElastic(m_AnimationFraction);

            for (SceneEl const& el : mg.iter())
            {
                size_t start = m_DrawablesBuffer.size();
                m_Shared->appendDrawables(el, m_DrawablesBuffer);
                size_t end = m_DrawablesBuffer.size();

                bool isSelectableEl = isSelectable(el);
                SceneDecorationFlags flags = computeFlags(el);

                for (size_t i = start; i < end; ++i)
                {
                    DrawableThing& d = m_DrawablesBuffer[i];
                    d.flags = flags;

                    if (!isSelectableEl)
                    {
                        d.color.a = fadedAlpha;
                        d.id = ModelGraphIDs::Empty();
                        d.groupId = ModelGraphIDs::Empty();
                    }
                    else
                    {
                        d.transform.scale *= animScale;
                    }
                }
            }

            // floor
            m_DrawablesBuffer.push_back(m_Shared->generateFloorDrawable());

            return m_DrawablesBuffer;
        }

        void handlePossibleCompletion()
        {
            if (static_cast<int>(m_SelectedEls.size()) < m_Options.numElementsUserMustChoose)
            {
                return;  // user hasn't selected enough stuff yet
            }

            if (m_Options.onUserChoice(m_SelectedEls))
            {
                requestPop();
            }
            else
            {
                // choice was rejected?
            }
        }

        // handle any side-effects from the user's mouse hover
        void handleHovertestSideEffects()
        {
            if (!m_MaybeHover)
            {
                return;
            }

            drawHoverTooltip();

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                tryToggleSelectionStateOf(m_MaybeHover.ID);
                handlePossibleCompletion();
            }
        }

        // draw 2D tooltip that pops up when user is hovered over something in the scene
        void drawHoverTooltip() const
        {
            if (!m_MaybeHover)
            {
                return;
            }

            SceneEl const* se = m_Shared->getModelGraph().tryGetElByID(m_MaybeHover.ID);

            if (se)
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(se->getLabel().c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("(%s, click to choose)", se->getClass().getName().c_str());
                ImGui::EndTooltip();
            }
        }

        // draw 2D connection overlay lines that show what's connected to what in the graph
        //
        // depends on layer options
        void drawConnectionLines() const
        {
            if (!m_MaybeHover)
            {
                // user isn't hovering anything, so just draw all existing connection
                // lines, but faintly
                m_Shared->drawConnectionLines(FaintifyColor(m_Shared->getColorConnectionLine()));
                return;
            }

            // else: user is hovering *something*

            // draw all other connection lines but exclude the thing being assigned (if any)
            m_Shared->drawConnectionLines(FaintifyColor(m_Shared->getColorConnectionLine()), m_Options.maybeElsBeingReplacedByChoice);

            // draw strong connection line between the things being attached to and the hover
            for (UID elAttachingTo : m_Options.maybeElsAttachingTo)
            {
                Vec3 parentPos = GetPosition(m_Shared->getModelGraph(), elAttachingTo);
                Vec3 childPos = GetPosition(m_Shared->getModelGraph(), m_MaybeHover.ID);

                if (!m_Options.isAttachingTowardEl)
                {
                    std::swap(parentPos, childPos);
                }

                ImU32 strongColorU2 = ImGui::ColorConvertFloat4ToU32(Vec4{m_Shared->getColorConnectionLine()});

                m_Shared->drawConnectionLine(strongColorU2, parentPos, childPos);
            }
        }

        // draw 2D header text in top-left corner of the screen
        void drawHeaderText() const
        {
            if (m_Options.header.empty())
            {
                return;
            }

            ImU32 color = ImGui::ColorConvertFloat4ToU32({1.0f, 1.0f, 1.0f, 1.0f});
            Vec2 padding = Vec2{10.0f, 10.0f};
            Vec2 pos = m_Shared->get3DSceneRect().p1 + padding;
            ImGui::GetWindowDrawList()->AddText(pos, color, m_Options.header.c_str());
        }

        // draw a user-clickable button for cancelling out of this choosing state
        void drawCancelButton()
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {10.0f, 10.0f});
            osc::PushStyleColor(ImGuiCol_Button, Color::halfGrey());

            CStringView const text = ICON_FA_ARROW_LEFT " Cancel (ESC)";
            Vec2 const margin = {25.0f, 35.0f};
            Vec2 const buttonTopLeft = m_Shared->get3DSceneRect().p2 - (osc::CalcButtonSize(text) + margin);

            ImGui::SetCursorScreenPos(buttonTopLeft);
            if (ImGui::Button(text.c_str()))
            {
                requestPop();
            }

            osc::PopStyleColor();
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

            if (m_AnimationFraction < 1.0f)
            {
                m_AnimationFraction = std::clamp(m_AnimationFraction + 0.5f*dt, 0.0f, 1.0f);
                App::upd().requestRedraw();
            }
        }

        void implOnDraw() final
        {
            m_Shared->setContentRegionAvailAsSceneRect();

            std::vector<DrawableThing>& drawables = generateDrawables();

            m_MaybeHover = m_Shared->doHovertest(drawables);
            handleHovertestSideEffects();

            m_Shared->drawScene(drawables);
            drawConnectionLines();
            drawHeaderText();
            drawCancelButton();
        }

        Color FaintifyColor(Color const& srcColor) const
        {
            Color color = srcColor;
            color.a *= 0.2f;
            return color;
        }

        // data that's shared between other UI states
        std::shared_ptr<MeshImporterSharedState> m_Shared;

        // options for this state
        ChooseElLayerOptions m_Options;

        // (maybe) user mouse hover
        MeshImporterHover m_MaybeHover;

        // elements selected by user
        std::vector<UID> m_SelectedEls;

        // buffer that's filled with drawable geometry during a drawcall
        std::vector<DrawableThing> m_DrawablesBuffer;

        // fraction that the system is through its animation cycle: ranges from 0.0 to 1.0 inclusive
        float m_AnimationFraction = 0.0f;
    };
}

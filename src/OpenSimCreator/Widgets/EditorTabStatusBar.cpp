#include "EditorTabStatusBar.hpp"

#include "OpenSimCreator/MiddlewareAPIs/EditorAPI.hpp"
#include "OpenSimCreator/Model/UndoableModelStatePair.hpp"
#include "OpenSimCreator/Widgets/ComponentContextMenu.hpp"
#include "OpenSimCreator/Utils/OpenSimHelpers.hpp"

#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <imgui.h>
#include <OpenSim/Common/Component.h>

#include <memory>
#include <utility>


class osc::EditorTabStatusBar::Impl final {
public:
    Impl(
        std::weak_ptr<MainUIStateAPI> mainUIStateAPI_,
        EditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> model_) :

        m_MainUIStateAPI{std::move(mainUIStateAPI_)},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(model_)}
    {
    }

    void onDraw()
    {
        osc::BeginMainViewportBottomBar("bottom");
        drawSelectionBreadcrumbs();
        ImGui::End();
    }

private:
    void drawSelectionBreadcrumbs()
    {
        OpenSim::Component const* const c = m_Model->getSelected();

        if (c)
        {
            std::vector<OpenSim::Component const*> const els = osc::GetPathElements(*c);
            for (int i = 0; i < static_cast<int>(els.size())-1; ++i)
            {
                ImGui::PushID(i);
                std::string const label = osc::Ellipsis(els[i]->getName(), 15);
                if (ImGui::SmallButton(label.c_str()))
                {
                    m_Model->setSelected(els[i]);
                }
                drawMouseInteractionStuff(*els[i]);
                ImGui::SameLine();
                ImGui::TextDisabled("/");
                ImGui::SameLine();
                ImGui::PopID();
            }
            if (!els.empty())
            {
                std::string const label = osc::Ellipsis(els.back()->getName(), 15);
                ImGui::TextUnformatted(label.c_str());
                drawMouseInteractionStuff(*els.back());
            }
        }
        else
        {
            ImGui::TextDisabled("(nothing selected)");
        }
    }

    void drawMouseInteractionStuff(OpenSim::Component const& c)
    {
        if (ImGui::IsItemHovered())
        {
            m_Model->setHovered(&c);

            osc::BeginTooltip();
            ImGui::TextDisabled("%s", c.getConcreteClassName().c_str());
            osc::EndTooltip();
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            auto menu = std::make_unique<ComponentContextMenu>("##hovermenu", m_MainUIStateAPI, m_EditorAPI, m_Model, osc::GetAbsolutePath(c));
            menu->open();
            m_EditorAPI->pushPopup(std::move(menu));
        }
    }

    std::weak_ptr<MainUIStateAPI> m_MainUIStateAPI;
    EditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
};

// public API (PIMPL)

osc::EditorTabStatusBar::EditorTabStatusBar(
    std::weak_ptr<MainUIStateAPI> mainUIStateAPI_,
    EditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(std::move(mainUIStateAPI_), editorAPI_, std::move(model_))}
{
}

osc::EditorTabStatusBar::EditorTabStatusBar(EditorTabStatusBar&&) noexcept = default;
osc::EditorTabStatusBar& osc::EditorTabStatusBar::operator=(EditorTabStatusBar&&) noexcept = default;
osc::EditorTabStatusBar::~EditorTabStatusBar() noexcept = default;

void osc::EditorTabStatusBar::onDraw()
{
    m_Impl->onDraw();
}

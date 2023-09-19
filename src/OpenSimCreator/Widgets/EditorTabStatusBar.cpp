#include "EditorTabStatusBar.hpp"

#include <OpenSimCreator/MiddlewareAPIs/EditorAPI.hpp>
#include <OpenSimCreator/Model/UndoableModelStatePair.hpp>
#include <OpenSimCreator/Widgets/ComponentContextMenu.hpp>
#include <OpenSimCreator/Utils/OpenSimHelpers.hpp>

#include <imgui.h>
#include <OpenSim/Common/Component.h>
#include <oscar/Bindings/ImGuiHelpers.hpp>
#include <oscar/Utils/ParentPtr.hpp>
#include <oscar/Utils/StringHelpers.hpp>

#include <memory>
#include <utility>


class osc::EditorTabStatusBar::Impl final {
public:
    Impl(
        ParentPtr<MainUIStateAPI> const& mainUIStateAPI_,
        EditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> model_) :

        m_MainUIStateAPI{mainUIStateAPI_},
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
            for (ptrdiff_t i = 0; i < osc::ssize(els)-1; ++i)
            {
                osc::PushID(i);
                std::string const label = osc::Ellipsis(els[i]->getName(), 15);
                if (ImGui::SmallButton(label.c_str()))
                {
                    m_Model->setSelected(els[i]);
                }
                drawMouseInteractionStuff(*els[i]);
                ImGui::SameLine();
                ImGui::TextDisabled("/");
                ImGui::SameLine();
                osc::PopID();
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
            auto menu = std::make_unique<ComponentContextMenu>(
                "##hovermenu",
                m_MainUIStateAPI,
                m_EditorAPI,
                m_Model,
                osc::GetAbsolutePath(c)
            );
            menu->open();
            m_EditorAPI->pushPopup(std::move(menu));
        }
    }

    ParentPtr<MainUIStateAPI> m_MainUIStateAPI;
    EditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
};

// public API (PIMPL)

osc::EditorTabStatusBar::EditorTabStatusBar(
    ParentPtr<MainUIStateAPI> const& mainUIStateAPI_,
    EditorAPI* editorAPI_,
    std::shared_ptr<UndoableModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(mainUIStateAPI_, editorAPI_, std::move(model_))}
{
}

osc::EditorTabStatusBar::EditorTabStatusBar(EditorTabStatusBar&&) noexcept = default;
osc::EditorTabStatusBar& osc::EditorTabStatusBar::operator=(EditorTabStatusBar&&) noexcept = default;
osc::EditorTabStatusBar::~EditorTabStatusBar() noexcept = default;

void osc::EditorTabStatusBar::onDraw()
{
    m_Impl->onDraw();
}

#include "EditorTabStatusBar.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/MiddlewareAPIs/EditorAPI.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Utils/Algorithms.hpp"
#include "src/Widgets/ComponentContextMenu.hpp"

#include <imgui.h>
#include <OpenSim/Common/Component.h>

#include <memory>
#include <utility>


class osc::EditorTabStatusBar::Impl final {
public:
    Impl(EditorAPI* api, std::shared_ptr<UndoableModelStatePair> model) :
        m_API{std::move(api)},
        m_Model{std::move(model)}
    {
    }

    void draw()
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
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextDisabled("%s", c.getConcreteClassName().c_str());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            auto menu = std::make_unique<ComponentContextMenu>("##hovermenu", m_API, m_Model, c.getAbsolutePath());
            menu->open();
            m_API->pushPopup(std::move(menu));
        }
    }

    EditorAPI* m_API;
    std::shared_ptr<UndoableModelStatePair> m_Model;
};

// public API

osc::EditorTabStatusBar::EditorTabStatusBar(EditorAPI* api, std::shared_ptr<UndoableModelStatePair> model) :
    m_Impl{new Impl{std::move(api), std::move(model)}}
{
}

osc::EditorTabStatusBar::EditorTabStatusBar(EditorTabStatusBar&& tmp) noexcept :
    m_Impl{std::exchange(tmp.m_Impl, nullptr)}
{
}

osc::EditorTabStatusBar& osc::EditorTabStatusBar::operator=(EditorTabStatusBar&& tmp) noexcept
{
    std::swap(m_Impl, tmp.m_Impl);
    return *this;
}

osc::EditorTabStatusBar::~EditorTabStatusBar() noexcept
{
    delete m_Impl;
}

void osc::EditorTabStatusBar::draw()
{
    m_Impl->draw();
}
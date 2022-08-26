#include "EditorTabStatusBar.hpp"

#include "src/Bindings/ImGuiHelpers.hpp"
#include "src/OpenSimBindings/OpenSimHelpers.hpp"
#include "src/OpenSimBindings/UndoableModelStatePair.hpp"
#include "src/Utils/Algorithms.hpp"

#include <imgui.h>
#include <OpenSim/Common/Component.h>

#include <memory>
#include <utility>


class osc::EditorTabStatusBar::Impl final {
public:
    Impl(std::shared_ptr<UndoableModelStatePair> model) :
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
                std::string const label = osc::Ellipsis(els[i]->getName(), 15);
                if (ImGui::SmallButton(label.c_str()))
                {
                    m_Model->setSelected(els[i]);
                }
                if (ImGui::IsItemHovered())
                {
                    m_Model->setHovered(els[i]);
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextDisabled("%s", els[i]->getConcreteClassName().c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::TextDisabled("/");
                ImGui::SameLine();
            }
            if (!els.empty())
            {
                std::string const label = osc::Ellipsis(els.back()->getName(), 15);
                ImGui::TextUnformatted(label.c_str());
            }
        }
        else
        {
            ImGui::TextDisabled("(nothing selected)");
        }
    }

    std::shared_ptr<UndoableModelStatePair> m_Model;
};

// public API

osc::EditorTabStatusBar::EditorTabStatusBar(std::shared_ptr<UndoableModelStatePair> model) :
    m_Impl{new Impl{std::move(model)}}
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
#include "EditorTabStatusBar.h"

#include <OpenSimCreator/Documents/Model/UndoableModelStatePair.h>
#include <OpenSimCreator/UI/ModelEditor/ComponentContextMenu.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <oscar/UI/ImGuiHelpers.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/ParentPtr.h>
#include <oscar/Utils/StringHelpers.h>

#include <memory>
#include <utility>


class osc::EditorTabStatusBar::Impl final {
public:
    Impl(
        ParentPtr<IMainUIStateAPI> const& mainUIStateAPI_,
        IEditorAPI* editorAPI_,
        std::shared_ptr<UndoableModelStatePair> model_) :

        m_MainUIStateAPI{mainUIStateAPI_},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(model_)}
    {
    }

    void onDraw()
    {
        ui::BeginMainViewportBottomBar("bottom");
        drawSelectionBreadcrumbs();
        ui::End();
    }

private:
    void drawSelectionBreadcrumbs()
    {
        OpenSim::Component const* const c = m_Model->getSelected();

        if (c)
        {
            std::vector<OpenSim::Component const*> const els = GetPathElements(*c);
            for (ptrdiff_t i = 0; i < std::ssize(els)-1; ++i)
            {
                ui::PushID(i);
                std::string const label = Ellipsis(els[i]->getName(), 15);
                if (ui::SmallButton(label))
                {
                    m_Model->setSelected(els[i]);
                }
                drawMouseInteractionStuff(*els[i]);
                ui::SameLine();
                ui::TextDisabled("/");
                ui::SameLine();
                ui::PopID();
            }
            if (!els.empty())
            {
                std::string const label = Ellipsis(els.back()->getName(), 15);
                ui::TextUnformatted(label);
                drawMouseInteractionStuff(*els.back());
            }
        }
        else
        {
            ui::TextDisabled("(nothing selected)");
        }
    }

    void drawMouseInteractionStuff(OpenSim::Component const& c)
    {
        if (ui::IsItemHovered())
        {
            m_Model->setHovered(&c);

            ui::BeginTooltip();
            ui::TextDisabled(c.getConcreteClassName());
            ui::EndTooltip();
        }
        if (ui::IsItemClicked(ImGuiMouseButton_Right))
        {
            auto menu = std::make_unique<ComponentContextMenu>(
                "##hovermenu",
                m_MainUIStateAPI,
                m_EditorAPI,
                m_Model,
                GetAbsolutePath(c)
            );
            menu->open();
            m_EditorAPI->pushPopup(std::move(menu));
        }
    }

    ParentPtr<IMainUIStateAPI> m_MainUIStateAPI;
    IEditorAPI* m_EditorAPI;
    std::shared_ptr<UndoableModelStatePair> m_Model;
};

// public API (PIMPL)

osc::EditorTabStatusBar::EditorTabStatusBar(
    ParentPtr<IMainUIStateAPI> const& mainUIStateAPI_,
    IEditorAPI* editorAPI_,
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

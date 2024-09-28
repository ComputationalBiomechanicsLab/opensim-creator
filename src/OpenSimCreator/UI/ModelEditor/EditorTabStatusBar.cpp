#include "EditorTabStatusBar.h"

#include <OpenSimCreator/Documents/Model/IModelStatePair.h>
#include <OpenSimCreator/UI/ModelEditor/ComponentContextMenu.h>
#include <OpenSimCreator/UI/ModelEditor/IEditorAPI.h>
#include <OpenSimCreator/Utils/OpenSimHelpers.h>

#include <OpenSim/Common/Component.h>
#include <oscar/UI/oscimgui.h>
#include <oscar/Utils/ParentPtr.h>
#include <oscar/Utils/StringHelpers.h>

#include <memory>
#include <utility>


class osc::EditorTabStatusBar::Impl final {
public:
    Impl(
        const ParentPtr<MainUIScreen>& mainUIStateAPI_,
        IEditorAPI* editorAPI_,
        std::shared_ptr<IModelStatePair> model_) :

        m_MainUIStateAPI{mainUIStateAPI_},
        m_EditorAPI{editorAPI_},
        m_Model{std::move(model_)}
    {}

    void onDraw()
    {
        ui::begin_main_viewport_bottom_bar("bottom");
        drawSelectionBreadcrumbs();
        ui::end_panel();
    }

private:
    void drawSelectionBreadcrumbs()
    {
        const OpenSim::Component* const c = m_Model->getSelected();

        if (c) {
            const std::vector<const OpenSim::Component*> els = GetPathElements(*c);
            for (ptrdiff_t i = 0; i < std::ssize(els)-1; ++i) {
                ui::push_id(i);
                const std::string label = truncate_with_ellipsis(els[i]->getName(), 15);
                if (ui::draw_small_button(label)) {
                    m_Model->setSelected(els[i]);
                }
                drawMouseInteractionStuff(*els[i]);
                ui::same_line();
                ui::draw_text_disabled("/");
                ui::same_line();
                ui::pop_id();
            }
            if (not els.empty()) {
                const std::string label = truncate_with_ellipsis(els.back()->getName(), 15);
                ui::draw_text_unformatted(label);
                drawMouseInteractionStuff(*els.back());
            }
        }
        else {
            ui::draw_text_disabled("(nothing selected)");
        }
    }

    void drawMouseInteractionStuff(const OpenSim::Component& c)
    {
        if (ui::is_item_hovered()) {
            m_Model->setHovered(&c);

            ui::begin_tooltip();
            ui::draw_text_disabled(c.getConcreteClassName());
            ui::end_tooltip();
        }
        if (ui::is_item_clicked(ui::MouseButton::Right)) {
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

    ParentPtr<MainUIScreen> m_MainUIStateAPI;
    IEditorAPI* m_EditorAPI;
    std::shared_ptr<IModelStatePair> m_Model;
};


osc::EditorTabStatusBar::EditorTabStatusBar(
    const ParentPtr<MainUIScreen>& mainUIStateAPI_,
    IEditorAPI* editorAPI_,
    std::shared_ptr<IModelStatePair> model_) :

    m_Impl{std::make_unique<Impl>(mainUIStateAPI_, editorAPI_, std::move(model_))}
{}
osc::EditorTabStatusBar::EditorTabStatusBar(EditorTabStatusBar&&) noexcept = default;
osc::EditorTabStatusBar& osc::EditorTabStatusBar::operator=(EditorTabStatusBar&&) noexcept = default;
osc::EditorTabStatusBar::~EditorTabStatusBar() noexcept = default;

void osc::EditorTabStatusBar::onDraw()
{
    m_Impl->onDraw();
}
